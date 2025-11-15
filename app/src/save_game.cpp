#include "save_game.h"
#include "json.hpp"
#include "core/fmemory.h"
#include "core/logger.h"

#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#include <array>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm> // For std::equal, std::copy

using json = nlohmann::json;

#define AES_KEY_SIZE_BYTES 16 // AES-128 (16 bytes)
#define AES_BLOCK_SIZE 16 // AES block size
#define AES_IV_SIZE 16 // CBC IV size (must be equal to AES_BLOCK_SIZE)
#define HMAC_TAG_SIZE 32 // HMAC-SHA256 always produces 32 bytes

#define JSON_SAVE_DATA_MAP_CURRENCY_COINS "currency_coins_player_have"
#define JSON_SAVE_DATA_VERSION "version"

#define JSON_SAVE_DATA_MAP_PLAYER_DATA "player_data"
#define JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY "inventory"
#define JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY_ITEM_TYPE "item_type"
#define JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY_IG_BUFFER "ig_buffer"

#define JSON_SAVE_DATA_MAP_SIGIL_SLOTS "sigil_slot"
#define JSON_SAVE_DATA_MAP_PLAYER_DATA_SIGIL_SLOTS_ITEM_TYPE "item_type"
#define JSON_SAVE_DATA_MAP_PLAYER_DATA_SIGIL_SLOTS_IG_BUFFER "ig_buffer"

#define JSON_SAVE_DATA_MAP_GAME_RULE "game_rule"
#define JSON_SAVE_DATA_MAP_GAME_RULE_SPAWN_MULTIPLIER "spawn_multiplier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_PLAY_TIME_MULTIPLIER "play_time_multiplier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_DELTA_TIME_MULTIPLIER "delta_time_multiplier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_BOSS_MODIFIER "boss_modifier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_AREA_UNLOCKER "area_unlocker"
#define JSON_SAVE_DATA_MAP_GAME_RULE_TRAIT_POINT_MODIFIER "trait_point_modifier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_BONUS_RESULT_MULTIPLIER "bonus_result_multiplier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_ZOMBIE_LEVEL_MODIFIER "zombie_level_modifier"
#define JSON_SAVE_DATA_MAP_GAME_RULE_RESERVED_FOR_FUTURE_USE "future_use"


#define SAVE_FILE_VERSION_051125 051125
#define SAVE_FILE_CURRENT_VERSION SAVE_FILE_VERSION_051125

// Save system state
struct save_game_system_state {
  std::array<save_data, SAVE_SLOT_MAX> save_slots;
  std::array<std::string, SAVE_SLOT_MAX> slot_filenames;

  save_game_system_state() {
    save_slots.fill(save_data());
    slot_filenames.fill("");
  }
};

static save_game_system_state* state = nullptr;

// --- CRYPTOGRAPHIC KEYS (MUST BE DIFFERENT) ---

// 1. Encryption Key (16 bytes for AES-128)
static const std::array<uint8_t, AES_KEY_SIZE_BYTES> aes_key = {
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

// 2. Authentication Key (32 bytes)
static const std::array<uint8_t, HMAC_TAG_SIZE> hmac_key = {
  0x6a, 0x9e, 0x3d, 0x77, 0x8b, 0x4f, 0x2c, 0x11,
  0x55, 0x01, 0xaf, 0xcc, 0xd0, 0x91, 0x3b, 0x7e,
  0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7,
  0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c, 0xde, 0xad
};

// Forward declarations
std::string get_save_filename(save_slot_id slot);
bool encrypt_data(const std::string& input, std::vector<uint8_t>& output);
bool decrypt_data(const std::vector<uint8_t>& input, std::string& output);

json serialize_save_data(const save_data& data);
void deserialize_save_data(const json& j, save_data& data);

json serialize_save_data_v051125(const save_data& data);
void deserialize_save_data_v051125(const json& j, save_data& data);

// --- OPENSSL HMAC IMPLEMENTATION ---
// Hash is calculated over the IV and the Ciphertext
std::vector<uint8_t> calculate_file_hmac(const std::vector<uint8_t>& data_to_hash) {
    std::vector<uint8_t> tag(HMAC_TAG_SIZE);
    unsigned int tag_len = 0;

    // Use HMAC_SHA256 from OpenSSL
    if (HMAC(EVP_sha256(), 
             hmac_key.data(), HMAC_TAG_SIZE, 
             data_to_hash.data(), data_to_hash.size(), 
             tag.data(), &tag_len) == nullptr) {
        IERROR("save_game::calculate_file_hmac()::OpenSSL HMAC failed.");
        return {}; 
    }

    if (tag_len != HMAC_TAG_SIZE) {
        IERROR("save_game::calculate_file_hmac()::HMAC size mismatch.");
        return {};
    }

    return tag;
}
// --- END OPENSSL HMAC IMPLEMENTATION ---

bool save_system_initialize() {
  if (state) {
    return true;
  }
  // Use fmemory.h for memory allocation
  state = (save_game_system_state*)allocate_memory_linear(sizeof(save_game_system_state), true);
  if (!state) {
    IFATAL("save_game::save_system_initialize()::Save system state allocation failed");
    return false;
  }
  *state = save_game_system_state();

  for (size_t i = 0; i < SAVE_SLOT_MAX; ++i) {
    state->slot_filenames[i] = get_save_filename(static_cast<save_slot_id>(i));
    state->save_slots[i].id = static_cast<save_slot_id>(i);
    state->save_slots[i].file_name = state->slot_filenames[i];
  }

  return true;
}

bool parse_or_create_save_data_from_file(save_slot_id slot, save_data default_save) {
  if (not state or state == nullptr) {
    IFATAL("save_game::parse_or_create_save_data_from_file()::Save game state is not valid");
    return false;
  }
  if (slot <= SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::parse_or_create_save_data_from_file()::Slot out of bound");
    return false;
  }
  save_data& save = state->save_slots[slot];
  save = default_save;
  save.id = slot;
  save.file_name = state->slot_filenames[slot];

  if (not FileExists(save.file_name.c_str())) {
    return save_save_data(slot);
  }

  int32_t out_datasize = 0;
  uint8_t* data = LoadFileData(save.file_name.c_str(), &out_datasize);
  // Check minimum required size: IV + 1 Block (Ciphertext) + HMAC Tag
  if (not data or out_datasize < (AES_IV_SIZE + AES_BLOCK_SIZE + HMAC_TAG_SIZE)) {
    IERROR("save_game::parse_or_create_save_data_from_file()::File too small or failed to load");
    UnloadFileData(data);
    return false;
  }

  std::vector<uint8_t> file_data(data, data + out_datasize);
  UnloadFileData(data);

  // 1. Separate Encrypted Data (IV + Ciphertext) from HMAC Tag
  size_t encrypted_data_size = file_data.size() - HMAC_TAG_SIZE;
  
  // Data to hash (IV + Ciphertext)
  std::vector<uint8_t> encrypted_data_and_iv(file_data.begin(), file_data.begin() + encrypted_data_size);
  // Stored HMAC tag
  std::vector<uint8_t> stored_tag(file_data.begin() + encrypted_data_size, file_data.end());

  // 2. Verify HMAC Tag (Integrity Check)
  std::vector<uint8_t> calculated_tag = calculate_file_hmac(encrypted_data_and_iv);
  
  // Note: std::equal is safe for cryptographic comparison on modern architectures
  if (calculated_tag.empty() || calculated_tag.size() != HMAC_TAG_SIZE || 
      !std::equal(calculated_tag.begin(), calculated_tag.end(), stored_tag.begin())) {
    IERROR("save_game::parse_or_create_save_data_from_file()::HMAC integrity check failed. File tampered or corrupted.");
    return false;
  }

  // 3. Decrypt Data
  std::string decrypted;
  if (!decrypt_data(encrypted_data_and_iv, decrypted)) {
    IERROR("save_game::parse_or_create_save_data_from_file()::Decryption failed");
    return false;
  }

  // 4. Parse JSON
  try {
    json j = json::parse(decrypted);
    deserialize_save_data(j, save);
    save.is_success = true;
  } catch (const json::exception& e) {
    IERROR("save_game::parse_or_create_save_data_from_file()::JSON parse error: %s", e.what());
    return false;
  }

  return true;
}

bool save_save_data(save_slot_id slot) {
  if (not state or state == nullptr) {
    IERROR("save_game::save_save_data()::Save game state is not valid");
    return false;
  }
  if (slot <= SAVE_SLOT_UNDEFINED or slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::save_save_data()::Slot out of bound");
    return false;
  }

  save_data* slot_data = &state->save_slots[slot];
  json j = serialize_save_data(*slot_data);
  std::string serialized = j.dump();

  // The EVP API handles PKCS#7 padding automatically if not explicitly disabled.
  // We'll calculate the maximum possible ciphertext size to pre-allocate.
  // Max size = Input size + Max padding (block size - 1)
  size_t max_ciphertext_size = serialized.size() + AES_BLOCK_SIZE;

  std::vector<uint8_t> encrypted_data_and_iv;
  // Pre-allocate space for IV + Max Ciphertext size
  encrypted_data_and_iv.resize(AES_IV_SIZE + max_ciphertext_size);

  if (!encrypt_data(serialized, encrypted_data_and_iv)) {
    IERROR("save_game::save_save_data()::Encryption failed");
    return false;
  }
  
  // 2. Calculate HMAC Tag
  // The HMAC is calculated over the IV and the Ciphertext
  // Note: encrypted_data_and_iv is now resized inside encrypt_data to actual size.
  std::vector<uint8_t> tag = calculate_file_hmac(encrypted_data_and_iv);
  if (tag.empty()) {
    IERROR("save_game::save_save_data()::HMAC tag calculation failed.");
    return false;
  }

  // 3. Combine IV, Ciphertext, and HMAC Tag for final file output
  std::vector<uint8_t> final_file_data = encrypted_data_and_iv;
  final_file_data.insert(final_file_data.end(), tag.begin(), tag.end());

  return SaveFileData(state->slot_filenames[slot].c_str(), final_file_data.data(), final_file_data.size());
}

// ---------------------------------------------------
// OPENSSL EVP AES-CBC ENCRYPTION/DECRYPTION IMPLEMENTATION
// ---------------------------------------------------

// Uses OpenSSL's EVP API
bool encrypt_data(const std::string& input, std::vector<uint8_t>& output) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        IERROR("save_game::encrypt_data()::EVP_CIPHER_CTX_new failed.");
        return false;
    }

    // 1. Generate IV
    std::vector<uint8_t> iv(AES_IV_SIZE);
    if (RAND_bytes(iv.data(), AES_IV_SIZE) != 1) {
        IERROR("save_game::encrypt_data()::OpenSSL RAND_bytes failed to generate IV.");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // 2. EVP Init for AES-128-CBC
    // We use the modern EVP_EncryptInit_ex
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, aes_key.data(), iv.data()) != 1) {
        IERROR("save_game::encrypt_data()::EVP_EncryptInit_ex failed.");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Since output was pre-allocated in save_save_data, we write the IV now
    std::copy(iv.begin(), iv.end(), output.begin());
    
    // Pointer to start of ciphertext area in output vector
    unsigned char* ciphertext_start = output.data() + AES_IV_SIZE;
    int len = 0;
    int ciphertext_total_len = 0;

    // 3. Encrypt Update
    // input.data() is the plaintext (including application-level padding if used, but EVP handles standard PKCS#7)
    if (EVP_EncryptUpdate(ctx, ciphertext_start, &len, (const unsigned char*)input.data(), input.size()) != 1) {
        IERROR("save_game::encrypt_data()::EVP_EncryptUpdate failed.");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_total_len = len;

    // 4. Encrypt Final (handles final block and padding)
    if (EVP_EncryptFinal_ex(ctx, ciphertext_start + len, &len) != 1) {
        IERROR("save_game::encrypt_data()::EVP_EncryptFinal_ex failed (Padding or key error).");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_total_len += len;
    
    // 5. Finalize and Resize
    EVP_CIPHER_CTX_free(ctx);
    
    // Resize the output vector to the actual data size (IV + Ciphertext)
    output.resize(AES_IV_SIZE + ciphertext_total_len);
    
    return true;
}

// Uses OpenSSL's EVP API
bool decrypt_data(const std::vector<uint8_t>& input, std::string& output) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        IERROR("save_game::decrypt_data()::EVP_CIPHER_CTX_new failed.");
        return false;
    }

    // Input here is the IV + Ciphertext (HMAC tag has been removed)
    size_t ciphertext_size = input.size() - AES_IV_SIZE;

    if (input.size() < AES_IV_SIZE || ciphertext_size % AES_BLOCK_SIZE != 0) {
        IERROR("save_game::decrypt_data()::Invalid input size or non-block aligned ciphertext.");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    const uint8_t* iv_ptr = input.data();
    const uint8_t* ciphertext_ptr = input.data() + AES_IV_SIZE;
    
    // 1. EVP Init for AES-128-CBC Decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, aes_key.data(), iv_ptr) != 1) {
        IERROR("save_game::decrypt_data()::EVP_DecryptInit_ex failed.");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // 2. Prepare output buffer for plaintext. 
    // Max size is always ciphertext_size.
    output.resize(ciphertext_size);
    unsigned char* plaintext_start = (unsigned char*)output.data();
    int len = 0;
    int plaintext_total_len = 0;

    // 3. Decrypt Update
    if (EVP_DecryptUpdate(ctx, plaintext_start, &len, ciphertext_ptr, ciphertext_size) != 1) {
        IERROR("save_game::decrypt_data()::EVP_DecryptUpdate failed.");
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_total_len = len;

    // 4. Decrypt Final (handles padding removal)
    if (EVP_DecryptFinal_ex(ctx, plaintext_start + len, &len) != 1) {
        IERROR("save_game::decrypt_data()::EVP_DecryptFinal_ex failed (Bad padding or MAC failure).");
        EVP_CIPHER_CTX_free(ctx);
        // Note: If EVP_DecryptFinal_ex fails, it usually indicates padding error
        // which strongly suggests a failed integrity check (tampering).
        return false;
    }
    plaintext_total_len += len;

    // 5. Finalize and Resize
    EVP_CIPHER_CTX_free(ctx);

    // Resize the output string to the actual plaintext length
    output.resize(plaintext_total_len);
    
    return true;
}


// --- REST OF THE FUNCTIONS (Unchanged logic) ---

bool does_save_exist(save_slot_id slot) {
  if (!state) {
    IERROR("save_game::does_save_exist()::State is not valid");
    return false;
  }
  if (slot < SAVE_SLOT_UNDEFINED || slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::does_save_exist()::Slot out of bound");
    return false;
  }

  return FileExists(state->slot_filenames[slot].c_str());
}

save_data* get_save_data(save_slot_id slot) {
  if (!state) {
    IERROR("save_game::get_save_data()::State is not valid");
    return nullptr;
  }
  if (slot < SAVE_SLOT_UNDEFINED || slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::get_save_data()::Slot out of bound");
    return nullptr;
  }

  return &state->save_slots[slot];
}

std::string get_save_filename(save_slot_id slot) {
  if (slot < SAVE_SLOT_UNDEFINED || slot >= SAVE_SLOT_MAX) {
    IWARN("save_game::get_save_filename()::Slot out of bound");
    return "";
  }
  if (slot == SAVE_SLOT_CURRENT_SESSION) {
    return TextFormat("save_slot_current%s", SAVE_GAME_EXTENSION);
  }
  return TextFormat("save_slot%d%s", static_cast<int32_t>(slot) + 1, SAVE_GAME_EXTENSION);
}

json serialize_save_data(const save_data& data) {

  return serialize_save_data_v051125(data);
}

void deserialize_save_data(const json& j, save_data& data) {
  data.player_data.inventory = std::vector<player_inventory_slot>();
  i32 version = 0;

  if (j.contains(JSON_SAVE_DATA_VERSION)) {
    version = j.value(JSON_SAVE_DATA_VERSION, -1);
  }
  if (version < 0) {
    data = save_data();
    IWARN("save_game::deserialize_save_data()::Version read failed");
    return;
  }
  else if (version == SAVE_FILE_VERSION_051125) {
    deserialize_save_data_v051125(j, data);
    return;
  }
  IERROR("save_game::deserialize_save_data()::Unsupported version");
}
json serialize_save_data_v051125(const save_data& data) {
  json j;
  j[JSON_SAVE_DATA_MAP_CURRENCY_COINS] = data.currency_coins_player_have;
  j[JSON_SAVE_DATA_VERSION] = SAVE_FILE_VERSION_051125;

  json rules;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_SPAWN_MULTIPLIER]            = data.game_rules.at(GAME_RULE_SPAWN_MULTIPLIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_PLAY_TIME_MULTIPLIER]        = data.game_rules.at(GAME_RULE_PLAY_TIME_MULTIPLIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_DELTA_TIME_MULTIPLIER]       = data.game_rules.at(GAME_RULE_DELTA_TIME_MULTIPLIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_BOSS_MODIFIER]               = data.game_rules.at(GAME_RULE_BOSS_MODIFIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_AREA_UNLOCKER]               = data.game_rules.at(GAME_RULE_AREA_UNLOCKER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_TRAIT_POINT_MODIFIER]        = data.game_rules.at(GAME_RULE_TRAIT_POINT_MODIFIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_BONUS_RESULT_MULTIPLIER]     = data.game_rules.at(GAME_RULE_BONUS_RESULT_MULTIPLIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_ZOMBIE_LEVEL_MODIFIER]       = data.game_rules.at(GAME_RULE_ZOMBIE_LEVEL_MODIFIER).level;
  rules[JSON_SAVE_DATA_MAP_GAME_RULE_RESERVED_FOR_FUTURE_USE]     = data.game_rules.at(GAME_RULE_RESERVED_FOR_FUTURE_USE).level;
  j[JSON_SAVE_DATA_MAP_GAME_RULE] = rules;

  json inventory;
  for (const player_inventory_slot& slot : data.player_data.inventory) {
    inventory.push_back({
      { JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY_ITEM_TYPE, static_cast<i32>(slot.item_type) },
      { JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY_IG_BUFFER, slot.ig_buffer.f32[1]},
    });
  }
  j[JSON_SAVE_DATA_MAP_PLAYER_DATA][JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY] = inventory;

  json sigil_slots;
  for (const sigil_slot& slot : data.sigil_slots) {
    sigil_slots.push_back({
      { JSON_SAVE_DATA_MAP_PLAYER_DATA_SIGIL_SLOTS_ITEM_TYPE, static_cast<i32>(slot.sigil.type) },
      { JSON_SAVE_DATA_MAP_PLAYER_DATA_SIGIL_SLOTS_IG_BUFFER, slot.sigil.buffer.f32[1]}
    });
  }
  j[JSON_SAVE_DATA_MAP_SIGIL_SLOTS] = sigil_slots;
  return j;
}

void deserialize_save_data_v051125(const json& j, save_data& data) {
  data.player_data.inventory = std::vector<player_inventory_slot>();

  if (j.contains(JSON_SAVE_DATA_MAP_CURRENCY_COINS)) {
    data.currency_coins_player_have = j.value(JSON_SAVE_DATA_MAP_CURRENCY_COINS, -1);
  }
  if (j.contains(JSON_SAVE_DATA_MAP_GAME_RULE)) {
    const auto& rules = j[JSON_SAVE_DATA_MAP_GAME_RULE];
    data.game_rules.at(GAME_RULE_SPAWN_MULTIPLIER).level        = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_SPAWN_MULTIPLIER,       -1);
    data.game_rules.at(GAME_RULE_PLAY_TIME_MULTIPLIER).level    = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_PLAY_TIME_MULTIPLIER,   -1);
    data.game_rules.at(GAME_RULE_DELTA_TIME_MULTIPLIER).level   = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_DELTA_TIME_MULTIPLIER,  -1);
    data.game_rules.at(GAME_RULE_BOSS_MODIFIER).level           = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_BOSS_MODIFIER,          -1);
    data.game_rules.at(GAME_RULE_AREA_UNLOCKER).level           = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_AREA_UNLOCKER,          -1);
    data.game_rules.at(GAME_RULE_TRAIT_POINT_MODIFIER).level    = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_TRAIT_POINT_MODIFIER,   -1);
    data.game_rules.at(GAME_RULE_BONUS_RESULT_MULTIPLIER).level = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_BONUS_RESULT_MULTIPLIER,-1);
    data.game_rules.at(GAME_RULE_ZOMBIE_LEVEL_MODIFIER).level   = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_ZOMBIE_LEVEL_MODIFIER,  -1);
    data.game_rules.at(GAME_RULE_RESERVED_FOR_FUTURE_USE).level = rules.value(JSON_SAVE_DATA_MAP_GAME_RULE_RESERVED_FOR_FUTURE_USE,-1);
  }
  if (j.contains(JSON_SAVE_DATA_MAP_PLAYER_DATA) && j[JSON_SAVE_DATA_MAP_PLAYER_DATA].contains(JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY)) {
    const auto& inventory = j[JSON_SAVE_DATA_MAP_PLAYER_DATA][JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY];

    for (const auto& raw_slot : inventory) {
      player_inventory_slot& slot = data.player_data.inventory.emplace_back(player_inventory_slot());
      slot.item_type = static_cast<item_type>(raw_slot.value(JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY_ITEM_TYPE, static_cast<i32>(ITEM_TYPE_UNDEFINED)));
      slot.ig_buffer.f32[1] = raw_slot.value(JSON_SAVE_DATA_MAP_PLAYER_DATA_INVENTORY_IG_BUFFER, -1.f);
    }
  }
  if (j.contains(JSON_SAVE_DATA_MAP_SIGIL_SLOTS)) {
    const auto& sigil_slots = j[JSON_SAVE_DATA_MAP_SIGIL_SLOTS];

    for (size_t itr_000 = 0u; itr_000 < data.sigil_slots.size(); ++itr_000) {
      sigil_slot slot = sigil_slot();
      slot.sigil.type = static_cast<item_type>(sigil_slots[itr_000].value(JSON_SAVE_DATA_MAP_PLAYER_DATA_SIGIL_SLOTS_ITEM_TYPE, static_cast<i32>(ITEM_TYPE_UNDEFINED)));
      slot.sigil.buffer.f32[1] = sigil_slots[itr_000].value(JSON_SAVE_DATA_MAP_PLAYER_DATA_SIGIL_SLOTS_IG_BUFFER, -1.f);
    }
  }
}
