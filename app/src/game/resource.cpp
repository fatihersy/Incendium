#include "resource.h"
#include <core/fmemory.h>
#include <core/logger.h>

#include <tools/pak_parser.h>
#include <game/spritesheet.h>

typedef struct resource_system_state {
  std::array<Texture2D, TEX_ID_MAX> textures;
  std::array<atlas_texture, ATLAS_TEX_ID_MAX> atlas_textures;
  std::array<spritesheet, SHEET_ID_SPRITESHEET_TYPE_MAX> sprites;
  std::array<Image, IMAGE_TYPE_MAX> images;
  std::array<tilesheet, TILESHEET_TYPE_MAX> tilesheets;

  std::vector<tilemap_prop_static> tilemap_props_trees;
  std::vector<tilemap_prop_static> tilemap_props_tombstones;
  std::vector<tilemap_prop_static> tilemap_props_stones;
  std::vector<tilemap_prop_static> tilemap_props_spikes;
  std::vector<tilemap_prop_static> tilemap_props_skulls;
  std::vector<tilemap_prop_static> tilemap_props_pillars;
  std::vector<tilemap_prop_static> tilemap_props_lamps;
  std::vector<tilemap_prop_static> tilemap_props_fence;
  std::vector<tilemap_prop_static> tilemap_props_details;
  std::vector<tilemap_prop_static> tilemap_props_candles;
  std::vector<tilemap_prop_static> tilemap_props_buildings;
  std::vector<tilemap_prop_sprite> tilemap_props_sprite;

  i32 next_prop_id;
  resource_system_state(void) {
    this->textures.fill(Texture2D {0u, 0, 0, 0, 0});
    this->atlas_textures.fill(atlas_texture());
    this->sprites.fill(spritesheet());
    this->images.fill(Image {nullptr, 0, 0, 0, 0});
    this->tilesheets.fill(tilesheet());

    this->tilemap_props_trees = std::vector<tilemap_prop_static>();
    this->tilemap_props_tombstones = std::vector<tilemap_prop_static>();
    this->tilemap_props_stones = std::vector<tilemap_prop_static>();
    this->tilemap_props_spikes = std::vector<tilemap_prop_static>();
    this->tilemap_props_skulls = std::vector<tilemap_prop_static>();
    this->tilemap_props_pillars = std::vector<tilemap_prop_static>();
    this->tilemap_props_lamps = std::vector<tilemap_prop_static>();
    this->tilemap_props_fence = std::vector<tilemap_prop_static>();
    this->tilemap_props_details = std::vector<tilemap_prop_static>();
    this->tilemap_props_candles = std::vector<tilemap_prop_static>();
    this->tilemap_props_buildings = std::vector<tilemap_prop_static>();
    this->tilemap_props_sprite = std::vector<tilemap_prop_sprite>();
    this->next_prop_id = 0;
  }
} resource_system_state;

static resource_system_state * state = nullptr;

#if USE_PAK_FORMAT
void load_texture_pak(pak_file_id pak_id, i32 file_id, bool resize, Vector2 new_size, texture_id _id);
bool load_image_pak(pak_file_id pak_id, i32 file_id, bool resize, Vector2 new_size, image_type type);
#else
void load_texture_disk(const char * _path, bool resize, Vector2 new_size, texture_id _id);
bool load_image_disk(const char * _path, bool resize, Vector2 new_size, image_type type);
#endif

void load_texture_from_atlas(atlas_texture_id _id, Rectangle texture_area);
void load_spritesheet(texture_id _source_tex, spritesheet_id handle_id, Vector2 offset, i32 _fps, i32 _frame_width, i32 _frame_height, i32 _total_row, i32 _total_col);
void load_tilesheet(tilesheet_type _sheet_sheet_type, atlas_texture_id _atlas_tex_id, i32 _tile_count_x, i32 _tile_count_y, i32 _tile_size);

bool resource_system_initialize(void) {
  if (state and state != nullptr) return false;
  state = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);
  if (not state or state == nullptr) {
    IERROR("resource::allocate_memory_linear()::State allocation failed");
    return false;
  }
  *state = resource_system_state();

  // NOTE: resource files inside the pak file
  #if USE_PAK_FORMAT
  load_texture_pak(PAK_FILE_ASSET2, PAK_FILE_ASSET2_ATLAS, false, VECTOR2(0.f, 0.f), TEX_ID_ASSET_ATLAS);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_WORLDMAP_IMAGE, false, VECTOR2(0.f, 0.f), TEX_ID_WORLDMAP_WO_CLOUDS);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_BLACK_BACKGROUND_IMAGE1, false, VECTOR2(0.f, 0.f), TEX_ID_BLACK_BACKGROUND_IMG1);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_BLACK_BACKGROUND_IMAGE2, false, VECTOR2(0.f, 0.f), TEX_ID_BLACK_BACKGROUND_IMG2);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SPRITESHEET_ZAP, false, VECTOR2(0.f, 0.f), TEX_ID_ZAP);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_ASSET1_SPIN_RESULT_STAR_105_105, false, VECTOR2(0.f, 0.f), TEX_ID_SPIN_RESULT_STAR_105_105);

  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_ABILITY_CD,                 false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_ABILITY_CD,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_AOE,                        false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_AOE,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_BASIC_ATTACK_DAMAGE,        false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_BASIC_ATTACK_DAMAGE,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_BASIC_ATTACK_SPEED,         false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_BASIC_ATTACK_SPEED,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_CONDITION_DURATION,         false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_CONDITION_DURATION,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_CRITICAL_CHANCE,            false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_CRITICAL_CHANCE,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_CRITICAL_DAMAGE,            false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_CRITICAL_DAMAGE,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_DAMAGE_DEFERRAL,            false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DAMAGE_DEFERRAL,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_DAMAGE_OVER_TIME,           false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DAMAGE_OVER_TIME,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_DAMAGE_REDUCTION,           false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DAMAGE_REDUCTION,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_DROP_RATE,                  false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DROP_RATE,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_EXP_GAIN,                   false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_EXP_GAIN,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_HEALTH,                     false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_HEALTH,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_HP_REGEN,                   false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_HP_REGEN,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_LETHAL_SIGIL_EFFECTIVENESS, false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_LETHAL_SIGIL_EFFECTIVENESS,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_MANA,                       false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_MANA,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_MANA_REGEN,                 false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_MANA_REGEN,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_MOVE_SPEED,                 false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_MOVE_SPEED,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_OVERALL_DAMAGE,             false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_OVERALL_DAMAGE,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_OVERALL_LUCK,               false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_OVERALL_LUCK,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_PROJECTILE_AMOUTH,          false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_PROJECTILE_AMOUTH,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_REWARD_MODIFIER,            false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_REWARD_MODIFIER,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_SIGIL_EFFECTIVENESS,        false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_SIGIL_EFFECTIVENESS,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_STAMINA_REGEN,              false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_STAMINA_REGEN,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_STAMINA,                    false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_STAMINA,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_TOTAL_TRAIT_POINT,          false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_TOTAL_TRAIT_POINT,);
  load_texture_pak(PAK_FILE_ASSET1, PAK_FILE_SIGIL_VITAL_SIGIL_EFFECTIVENESS,  false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_VITAL_SIGIL_EFFECTIVENESS,);
  #else
    load_texture_disk("atlas.png", false, VECTOR2(0.f, 0.f), TEX_ID_ASSET_ATLAS);
    load_texture_disk("worldmap_wo_clouds.png", false, VECTOR2(0.f, 0.f), TEX_ID_WORLDMAP_WO_CLOUDS);
    load_texture_disk("black_background_1.png", false, VECTOR2(0.f, 0.f), TEX_ID_BLACK_BACKGROUND_IMG1);
    load_texture_disk("black_background_2.png", false, VECTOR2(0.f, 0.f), TEX_ID_BLACK_BACKGROUND_IMG2);
    load_texture_disk("zap.png", false, VECTOR2(0.f, 0.f), TEX_ID_ZAP);
    load_texture_disk("spin_result_star_105x105.png", false, VECTOR2(0.f, 0.f), TEX_ID_SPIN_RESULT_STAR_105_105);

    load_texture_disk("sigils/ability_cd_clear-modified.png",                false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_ABILITY_CD);
    load_texture_disk("sigils/aoe_clear-modified.png",                       false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_AOE);
    load_texture_disk("sigils/basic_attack_damage_clear-modified.png",       false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_BASIC_ATTACK_DAMAGE);
    load_texture_disk("sigils/basic_attack_speed_clear-modified.png",        false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_BASIC_ATTACK_SPEED);
    load_texture_disk("sigils/condition_duration_clear-modified.png",        false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_CONDITION_DURATION);
    load_texture_disk("sigils/critical_chance_clear-modified.png",           false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_CRITICAL_CHANCE);
    load_texture_disk("sigils/critical_damage_clear-modified.png",           false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_CRITICAL_DAMAGE);
    load_texture_disk("sigils/damage_deferral_clear-modified.png",           false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DAMAGE_DEFERRAL);
    load_texture_disk("sigils/damage_over_time_clear-modified.png",          false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DAMAGE_OVER_TIME);
    load_texture_disk("sigils/damage_reduction_clear-modified.png",          false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DAMAGE_REDUCTION);
    load_texture_disk("sigils/drop_rate_clear-modified.png",                 false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_DROP_RATE);
    load_texture_disk("sigils/exp_gain_clear-modified.png",                  false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_EXP_GAIN);
    load_texture_disk("sigils/health_sigil_clear-modified.png",              false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_HEALTH);
    load_texture_disk("sigils/hp_regen_clear-modified.png",                  false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_HP_REGEN);
    load_texture_disk("sigils/letal_sigil_effectiveness_clear-modified.png", false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_LETHAL_SIGIL_EFFECTIVENESS);
    load_texture_disk("sigils/mana_clear-modified.png",                      false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_MANA);
    load_texture_disk("sigils/mana_regen_clear-modified.png",                false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_MANA_REGEN);
    load_texture_disk("sigils/move_speed_clear-modified.png",                false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_MOVE_SPEED);
    load_texture_disk("sigils/overall_damage_clear-modified.png",            false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_OVERALL_DAMAGE);
    load_texture_disk("sigils/overall_luck_clear-modified.png",              false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_OVERALL_LUCK);
    load_texture_disk("sigils/projectile_amouth_clear-modified.png",         false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_PROJECTILE_AMOUTH);
    load_texture_disk("sigils/reward_modifier_clear-modified.png",           false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_REWARD_MODIFIER);
    load_texture_disk("sigils/sigil_effectiveness_clear-modified.png",       false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_SIGIL_EFFECTIVENESS);
    load_texture_disk("sigils/stamina_regen_clear-modified.png",             false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_STAMINA_REGEN);
    load_texture_disk("sigils/stamina_sigil_clear-modified.png",             false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_STAMINA);
    load_texture_disk("sigils/total_trait_points_clear-modified.png",        false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_TOTAL_TRAIT_POINT);
    load_texture_disk("sigils/vital_sigil_effectiveness_clear-modified.png", false, VECTOR2(0.f, 0.f), TEX_ID_SIGIL_VITAL_SIGIL_EFFECTIVENESS);
  #endif

  load_texture_from_atlas(ATLAS_TEX_ID_MAP_TILESET_TEXTURE,                  Rectangle{    0,    0, 1568, 2016 });
  load_texture_from_atlas(ATLAS_TEX_ID_INQUISITOR_PORTRAIT,                  Rectangle{ 1936,    0,   16,   16 });
  load_texture_from_atlas(ATLAS_TEX_ID_PORTRAIT_FRAME,                       Rectangle{ 1952,    0,   36,   34 });
  load_texture_from_atlas(ATLAS_TEX_ID_ABILITY_SLOT_FRAME,                   Rectangle{ 2000,    0,   16,   16 });
  load_texture_from_atlas(ATLAS_TEX_ID_ARROW,                                Rectangle{ 1696,    0,   11,   11 });
  load_texture_from_atlas(ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,            Rectangle{ 1568,  112,   48,    4 });
  load_texture_from_atlas(ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL,             Rectangle{ 1504,  112,   58,    4 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_OUTSIDE,       Rectangle{ 1680, 1088,  192,    7 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_INSIDE_YELLOW, Rectangle{ 1680, 1104,  192,    7 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_INSIDE_WHITE,  Rectangle{ 1680, 1111,  192,    7 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_MIDDLE,        Rectangle{ 1680, 1120,   32,    7 });
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_ORNATE_FRAME,         Rectangle{ 1568, 1232,   68,   68 });
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,                Rectangle{ 1504,   32,   48,   48 });
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,             Rectangle{ 1552,   32,   48,   48 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_PANEL,                   Rectangle{ 1728,    0,   80,   64 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_PANEL_SELECTED,          Rectangle{ 1808,    0,   80,   64 });
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG,                Rectangle{ 1888,    0,   48,   44 });
  load_texture_from_atlas(ATLAS_TEX_ID_BG_BLACK,                             Rectangle{ 1680,   32,   48,   48 });
  load_texture_from_atlas(ATLAS_TEX_ID_ZOMBIES_SPRITESHEET,                  Rectangle{ 2224,    0,  256,  480 });
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE,             Rectangle{ 1568, 1088,   68,   68 });
  load_texture_from_atlas(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR,            Rectangle{ 1648, 1088,   28,   28 });
  load_texture_from_atlas(ATLAS_TEX_ID_CURRENCY_SOUL_ICON,                   Rectangle{ 1888,  736,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_CURRENCY_COIN_ICON,                   Rectangle{ 1920,  736,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_FOG,                                  Rectangle{ 1152, 3296,  400,   64 });
  load_texture_from_atlas(ATLAS_TEX_ID_HEADER,                               Rectangle{ 1824,   64,  192,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_LITTLE_SHOWCASE,                      Rectangle{ 1504,  128,   88,   16 });
  load_texture_from_atlas(ATLAS_TEX_ID_PANEL_SCROLL,                         Rectangle{ 1504,  144,   16,   48 });
  load_texture_from_atlas(ATLAS_TEX_ID_PANEL_SCROLL_HANDLE,                  Rectangle{ 1520,  144,   16,   16 });
  load_texture_from_atlas(ATLAS_TEX_ID_CHEST_BASE,                           Rectangle{ 1568, 1200,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_CHEST_LID,                            Rectangle{ 1600, 1200,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_DAMAGE_COMMON,                  Rectangle{ 1888,  816,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_DAMAGE_UNCOMMON,                Rectangle{ 1920,  816,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_DAMAGE_RARE,                    Rectangle{ 1952,  816,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_DAMAGE_EPIC,                    Rectangle{ 1984,  816,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_RESISTANCE_COMMON,              Rectangle{ 1888,  848,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_RESISTANCE_UNCOMMON,            Rectangle{ 1920,  848,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_RESISTANCE_RARE,                Rectangle{ 1952,  848,   32,   32 });
  load_texture_from_atlas(ATLAS_TEX_ID_SIGIL_RESISTANCE_EPIC,                Rectangle{ 1984,  848,   32,   32 });

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT,        VECTOR2(1584,  224), 10,   22,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT,        VECTOR2(1584,  160), 10,   22,  32, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1584,  192), 13,   22,  32, 1,  3);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT,       VECTOR2(1632,   96),  9,   90, 110, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_ATTACK_1,          VECTOR2(1568, 1632), 15,  144,  64, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_ATTACK_2,          VECTOR2(1568, 1696), 15,  144,  64, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_ATTACK_3,          VECTOR2(1568, 1760), 10,  144,  64, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_ATTACK_DOWN,       VECTOR2(1568, 1952), 12,  144,  64, 1,  5);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_ATTACK_UP,         VECTOR2(1568, 2016), 12,  144,  64, 1,  5);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_ROLL,              VECTOR2(1568, 1824), 11,  144,  64, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_DASH,              VECTOR2(1568, 1888), 10,  144,  64, 1,  7);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_LEFT,          VECTOR2(2032,  16),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_RIGHT,         VECTOR2(2032, 176),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,   VECTOR2(1776, 176), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,  VECTOR2(1936, 176), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_LEFT,         VECTOR2(2032,  56),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_RIGHT,        VECTOR2(2032, 216),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1776, 216), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1936, 216), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_LEFT,         VECTOR2(2032,  96),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_RIGHT,        VECTOR2(2032, 256),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1776, 256), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1936, 256), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT,            VECTOR2(2032, 136),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT,           VECTOR2(2032, 296),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,     VECTOR2(1776, 296), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,    VECTOR2(1936, 296), 15,   32,  40, 1,  4);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_EXPLOSION, VECTOR2(1808, 528), 5,  32,  32, 1,  4);

  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_SLIDER_OPTION,                      VECTOR2(1632.f,  32.f),  0,  16,  10, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_SLIDER_LEFT_BUTTON,                 VECTOR2(1600.f,  32.f),  0,  12,  12, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_SLIDER_RIGHT_BUTTON,                VECTOR2(1600.f,  44.f),  0,  12,  12, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_MENU_BUTTON,                        VECTOR2(1505.f,   0.f),  0,  88,  13, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_FLAT_BUTTON,                        VECTOR2(1504.f,  16.f),  0,  88,  13, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_SLIDER_PERCENT,                     VECTOR2(1600.f,  64.f),  0,   9,   9, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_FLAME_ENERGY_ANIMATION,             VECTOR2(1584.f, 352.f), 13,  48,  48, 1, 18);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_FIREBALL_ANIMATION,                 VECTOR2(1584.f, 400.f), 10,  64,  64, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_FIREBALL_EXPLOTION_ANIMATION,       VECTOR2(1584.f, 464.f), 12,  64,  64, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_ABILITY_FIRETRAIL_START_ANIMATION,  VECTOR2(1584.f, 528.f), 15,  24,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_ABILITY_FIRETRAIL_LOOP_ANIMATION,   VECTOR2(1584.f, 560.f), 10,  24,  32, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS,              SHEET_ID_ABILITY_FIRETRAIL_END_ANIMATION,    VECTOR2(1680.f, 528.f), 15,  24,  32, 1,  5);
  load_spritesheet(TEX_ID_ZAP,                      SHEET_ID_ABILITY_CODEX,                      VECTOR2(   0.f,   0.f),240, 517, 517, 1, 60);
  load_spritesheet(TEX_ID_SPIN_RESULT_STAR_105_105, SHEET_ID_SPIN_RESULT_STAR_105_105,           VECTOR2(   0.f,   0.f), 60, 105, 105, 6,  9);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_LOOT_ITEM_HEALTH,     VECTOR2(1888,  768), 12,  16,  16, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_LOOT_ITEM_COIN,       VECTOR2(1888,  784), 12,  16,  16, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_LOOT_ITEM_EXPERIENCE, VECTOR2(1888,  800),  8,  16,  16, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_LOOT_ITEM_CHEST,      VECTOR2(1568, 1168), 15,  32,  32, 1,  7);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_ENVIRONMENTAL_PARTICLES, VECTOR2( 576, 3456), 5, 160,  96, 1, 12);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_LIGHT_INSECTS,           VECTOR2(1248, 2400), 5,  58,  71, 1, 12);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_CANDLE_FX,               VECTOR2(1088, 3360), 5,  64,  64, 1, 12);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_GENERIC_LIGHT,           VECTOR2(1248, 2272), 4, 128, 128, 1,  4);
  
  load_tilesheet(TILESHEET_TYPE_MAP, ATLAS_TEX_ID_MAP_TILESET_TEXTURE, 49, 63, 32);

  // Prop init
  {
    // Trees
    {
      add_prop_tree(Rectangle{  11, 2056, 166, 152});
      add_prop_tree(Rectangle{ 193, 2117, 122,  91});
      add_prop_tree(Rectangle{ 321, 2059, 127, 149});
      add_prop_tree(Rectangle{ 457, 2123,  53,  85});
      add_prop_tree(Rectangle{ 523, 2056, 166, 152});
      add_prop_tree(Rectangle{ 705, 2117, 122,  91});
      add_prop_tree(Rectangle{ 833, 2059, 127, 149});
      add_prop_tree(Rectangle{ 969, 2123,  53,  85});
      add_prop_tree(Rectangle{ 197, 2208,  84,  62});
      add_prop_tree(Rectangle{ 292, 2224,  57,  38});
      add_prop_tree(Rectangle{ 354, 2225,  60,  39});
      add_prop_tree(Rectangle{ 709, 2208,  84,  62});
      add_prop_tree(Rectangle{ 709, 2208,  84,  62});
      add_prop_tree(Rectangle{ 320,  256,  64,  32});
      add_prop_tree(Rectangle{ 804, 2224,  57,  38});
      add_prop_tree(Rectangle{ 866, 2225,  60,  39});
      add_prop_tree(Rectangle{ 288, 2272,  32,  32});
      add_prop_tree(Rectangle{ 320, 2272,  64,  32});
      add_prop_tree(Rectangle{ 384, 2272,  32,  32});
      add_prop_tree(Rectangle{ 416, 2272,  32,  32});
      add_prop_tree(Rectangle{ 800, 2272,  32,  32});
      add_prop_tree(Rectangle{ 832, 2272,  64,  32});
      add_prop_tree(Rectangle{ 896, 2272,  32,  32});
    }
    // Trees

    // Tombstones
    {
      add_prop_tombstone(Rectangle{  0, 2400, 32, 32});
      add_prop_tombstone(Rectangle{ 32, 2400, 32, 32});
      add_prop_tombstone(Rectangle{ 64, 2400, 32, 32});
      add_prop_tombstone(Rectangle{ 96, 2400, 32, 32});
      add_prop_tombstone(Rectangle{128, 2400, 32, 32});
      add_prop_tombstone(Rectangle{160, 2400, 32, 32});
      add_prop_tombstone(Rectangle{192, 2400, 32, 32});
      add_prop_tombstone(Rectangle{224, 2400, 32, 32});
      add_prop_tombstone(Rectangle{256, 2400, 32, 32});
      add_prop_tombstone(Rectangle{288, 2400, 32, 32});
      add_prop_tombstone(Rectangle{320, 2400, 32, 32});
      add_prop_tombstone(Rectangle{352, 2400, 32, 32});
      add_prop_tombstone(Rectangle{384, 2400, 32, 32});
      add_prop_tombstone(Rectangle{416, 2400, 32, 32});
      add_prop_tombstone(Rectangle{448, 2400, 32, 32});
      add_prop_tombstone(Rectangle{  0, 2432, 32, 64});
      add_prop_tombstone(Rectangle{ 32, 2432, 32, 64});
      add_prop_tombstone(Rectangle{ 64, 2432, 32, 32});
      add_prop_tombstone(Rectangle{ 96, 2432, 32, 32});
      add_prop_tombstone(Rectangle{128, 2432, 32, 32});
      add_prop_tombstone(Rectangle{160, 2432, 32, 32});
      add_prop_tombstone(Rectangle{192, 2432, 32, 32});
      add_prop_tombstone(Rectangle{224, 2432, 32, 32});
      add_prop_tombstone(Rectangle{256, 2432, 32, 32});
      add_prop_tombstone(Rectangle{288, 2432, 32, 32});
      add_prop_tombstone(Rectangle{320, 2432, 32, 32});
      add_prop_tombstone(Rectangle{352, 2432, 32, 32});
      add_prop_tombstone(Rectangle{384, 2432, 32, 32});
      add_prop_tombstone(Rectangle{416, 2432, 32, 32});
      add_prop_tombstone(Rectangle{ 64, 2464, 32, 32});
      add_prop_tombstone(Rectangle{ 96, 2464, 32, 32});
      add_prop_tombstone(Rectangle{128, 2464, 32, 32});
      add_prop_tombstone(Rectangle{160, 2464, 32, 32});
      add_prop_tombstone(Rectangle{192, 2464, 32, 32});
      add_prop_tombstone(Rectangle{224, 2464, 32, 32});
      add_prop_tombstone(Rectangle{256, 2464, 32, 32});
      add_prop_tombstone(Rectangle{288, 2464, 32, 32});
      add_prop_tombstone(Rectangle{320, 2464, 32, 32});
      add_prop_tombstone(Rectangle{352, 2464, 32, 32});
      add_prop_tombstone(Rectangle{384, 2464, 32, 32});
      add_prop_tombstone(Rectangle{416, 2464, 32, 32});
    }
    // Tombstones

    // Stones
    {
      add_prop_stone(Rectangle{1024, 2272,  64,  32});
      add_prop_stone(Rectangle{1088, 2272,  32,  32});
      add_prop_stone(Rectangle{1120, 2272,  64,  32});
      add_prop_stone(Rectangle{1184, 2272,  32,  32});
      add_prop_stone(Rectangle{1216, 2272,  32,  32});
      add_prop_stone(Rectangle{1024, 2304,  32,  32});
      add_prop_stone(Rectangle{1088, 2304,  32,  32});
      add_prop_stone(Rectangle{1120, 2304,  32,  32});
      add_prop_stone(Rectangle{1152, 2304,  32,  32});
      add_prop_stone(Rectangle{1184, 2304,  32,  32});
      add_prop_stone(Rectangle{1216, 2304,  32,  32});
      add_prop_stone(Rectangle{1056, 2336,  64,  48});
      add_prop_stone(Rectangle{1152, 2336,  32,  32});
      add_prop_stone(Rectangle{1184, 2336,  32,  32});
      add_prop_stone(Rectangle{1216, 2368,  32,  32});
      add_prop_stone(Rectangle{1024, 2400,  64,  32});
      add_prop_stone(Rectangle{1120, 2400,  64,  32});
      add_prop_stone(Rectangle{1024, 2432,  32,  32});
      add_prop_stone(Rectangle{1088, 2432,  32,  32});
      add_prop_stone(Rectangle{1120, 2432,  32,  32});
      add_prop_stone(Rectangle{1152, 2432,  32,  32});
      add_prop_stone(Rectangle{1184, 2432,  32,  32});
      add_prop_stone(Rectangle{1056, 2464,  64,  64});
      add_prop_stone(Rectangle{1120, 2464,  32,  32});
      add_prop_stone(Rectangle{1184, 2464,  32,  32});
      add_prop_stone(Rectangle{1216, 2496,   32, 32});
      add_prop_stone(Rectangle{1024, 2528,  64,  32});
      add_prop_stone(Rectangle{1088, 2528,  32,  32});
      add_prop_stone(Rectangle{1120, 2528,  64,  32});
      add_prop_stone(Rectangle{1184, 2528,  32,  32});
      add_prop_stone(Rectangle{1024, 2560,  32,  32});
      add_prop_stone(Rectangle{1088, 2560,  32,  32});
      add_prop_stone(Rectangle{1120, 2560,  32,  32});
      add_prop_stone(Rectangle{1184, 2560,  32,  32});
      add_prop_stone(Rectangle{1024, 2592,  32,  32});
      add_prop_stone(Rectangle{1056, 2592,  32,  32});
      add_prop_stone(Rectangle{1120, 2592,  32,  32});
      add_prop_stone(Rectangle{1184, 2592,  32,  32});
      add_prop_stone(Rectangle{1216, 2624,  32,  32});
      add_prop_stone(Rectangle{1024, 2656,  64,  32});
      add_prop_stone(Rectangle{1088, 2656,  32,  32});
      add_prop_stone(Rectangle{1120, 2656,  64,  32});
      add_prop_stone(Rectangle{1024, 2688,  32,  32});
      add_prop_stone(Rectangle{1120, 2688,  32,  32});
      add_prop_stone(Rectangle{1152, 2688,  32,  32});
      add_prop_stone(Rectangle{1184, 2688,  32,  32});
      add_prop_stone(Rectangle{1216, 2688,  32,  32});
      add_prop_stone(Rectangle{1024, 2720,  32,  32});
      add_prop_stone(Rectangle{1056, 2720,  64,  48});
      add_prop_stone(Rectangle{1216, 2752,  32,  32});
    }
    // Stones

    // Spikes
    {
      add_prop_spike(Rectangle{ 928, 3232, 32, 64});
      add_prop_spike(Rectangle{ 960, 3232, 32, 64});
      add_prop_spike(Rectangle{ 992, 3232, 32, 64});
      add_prop_spike(Rectangle{1024, 3232, 32, 64});
      add_prop_spike(Rectangle{1056, 3232, 32, 64});
      add_prop_spike(Rectangle{1088, 3232, 32, 64});
      add_prop_spike(Rectangle{1120, 3232, 32, 64});
      add_prop_spike(Rectangle{ 928, 3296, 32, 64});
      add_prop_spike(Rectangle{ 960, 3296, 32, 64});
      add_prop_spike(Rectangle{ 992, 3296, 32, 64});
      add_prop_spike(Rectangle{1024, 3296, 32, 64});
      add_prop_spike(Rectangle{1056, 3296, 32, 64});
      add_prop_spike(Rectangle{1088, 3296, 32, 64});
      add_prop_spike(Rectangle{1120, 3296, 32, 64});
      add_prop_spike(Rectangle{ 928, 3360, 32, 32});
      add_prop_spike(Rectangle{ 960, 3360, 32, 32});
      add_prop_spike(Rectangle{ 992, 3360, 32, 32});
      add_prop_spike(Rectangle{1024, 3360, 32, 32});
      add_prop_spike(Rectangle{1056, 3360, 32, 32});
    } 
    // Spikes

    // Skulls
    {
      add_prop_skull(Rectangle{  0, 3104, 32, 32});
      add_prop_skull(Rectangle{ 32, 3104, 32, 32});
      add_prop_skull(Rectangle{ 64, 3104, 32, 32});
      add_prop_skull(Rectangle{ 96, 3104, 32, 32});
      add_prop_skull(Rectangle{128, 3104, 32, 32});
      add_prop_skull(Rectangle{160, 3104, 32, 32});
      add_prop_skull(Rectangle{192, 3104, 32, 32});
      add_prop_skull(Rectangle{224, 3104, 32, 32});
      add_prop_skull(Rectangle{256, 3104, 32, 32});
      add_prop_skull(Rectangle{288, 3104, 32, 32});
      add_prop_skull(Rectangle{  0, 3136, 32, 32});
      add_prop_skull(Rectangle{ 32, 3136, 32, 32});
      add_prop_skull(Rectangle{ 64, 3136, 32, 32});
      add_prop_skull(Rectangle{ 96, 3136, 32, 32});
      add_prop_skull(Rectangle{128, 3136, 32, 32});
      add_prop_skull(Rectangle{160, 3136, 32, 32});
      add_prop_skull(Rectangle{192, 3136, 32, 32});
      add_prop_skull(Rectangle{224, 3136, 32, 32});
      add_prop_skull(Rectangle{256, 3136, 32, 32});
      add_prop_skull(Rectangle{288, 3136, 32, 32});
      add_prop_skull(Rectangle{  0, 3168, 32, 32});
      add_prop_skull(Rectangle{ 32, 3168, 32, 32});
      add_prop_skull(Rectangle{ 64, 3168, 32, 32});
      add_prop_skull(Rectangle{ 96, 3168, 32, 32});
      add_prop_skull(Rectangle{128, 3168, 32, 32});
      add_prop_skull(Rectangle{160, 3168, 32, 32});
      add_prop_skull(Rectangle{192, 3168, 32, 32});
      add_prop_skull(Rectangle{224, 3168, 32, 32});
      add_prop_skull(Rectangle{256, 3168, 32, 32});
      add_prop_skull(Rectangle{288, 3168, 32, 32});
      add_prop_skull(Rectangle{  0, 3200, 32, 32});
      add_prop_skull(Rectangle{ 32, 3200, 32, 32});
      add_prop_skull(Rectangle{ 64, 3200, 32, 32});
      add_prop_skull(Rectangle{ 96, 3200, 32, 32});
      add_prop_skull(Rectangle{128, 3200, 32, 32});
      add_prop_skull(Rectangle{160, 3200, 32, 32});
      add_prop_skull(Rectangle{192, 3200, 32, 32});
      add_prop_skull(Rectangle{224, 3200, 32, 32});
      add_prop_skull(Rectangle{256, 3200, 32, 32});
      add_prop_skull(Rectangle{288, 3200, 32, 32});
      add_prop_skull(Rectangle{  0, 3232, 32, 32});
      add_prop_skull(Rectangle{ 32, 3232, 32, 32});
      add_prop_skull(Rectangle{ 64, 3232, 32, 32});
      add_prop_skull(Rectangle{ 96, 3232, 32, 32});
      add_prop_skull(Rectangle{128, 3232, 32, 32});
      add_prop_skull(Rectangle{160, 3232, 32, 32});
      add_prop_skull(Rectangle{192, 3232, 32, 32});
      add_prop_skull(Rectangle{224, 3232, 32, 32});
      add_prop_skull(Rectangle{256, 3232, 32, 32});
      add_prop_skull(Rectangle{288, 3232, 32, 32});
      add_prop_skull(Rectangle{  0, 3264, 32, 32});
      add_prop_skull(Rectangle{ 32, 3264, 32, 32});
      add_prop_skull(Rectangle{ 64, 3264, 32, 32});
      add_prop_skull(Rectangle{ 96, 3264, 32, 32});
      add_prop_skull(Rectangle{128, 3264, 32, 32});
      add_prop_skull(Rectangle{160, 3264, 32, 32});
      add_prop_skull(Rectangle{192, 3264, 32, 32});
      add_prop_skull(Rectangle{224, 3264, 32, 32});
      add_prop_skull(Rectangle{256, 3264, 32, 32});
      add_prop_skull(Rectangle{288, 3264, 32, 32});
      add_prop_skull(Rectangle{  0, 3296, 32, 32});
      add_prop_skull(Rectangle{ 32, 3296, 32, 32});
      add_prop_skull(Rectangle{ 64, 3296, 32, 32});
      add_prop_skull(Rectangle{ 96, 3296, 32, 32});
      add_prop_skull(Rectangle{128, 3296, 32, 32});
      add_prop_skull(Rectangle{160, 3296, 32, 32});
      add_prop_skull(Rectangle{192, 3296, 32, 32});
      add_prop_skull(Rectangle{224, 3296, 32, 32});
      add_prop_skull(Rectangle{256, 3296, 32, 32});
      add_prop_skull(Rectangle{288, 3296, 32, 32});
      add_prop_skull(Rectangle{  0, 3328, 32, 32});
      add_prop_skull(Rectangle{ 32, 3328, 32, 32});
      add_prop_skull(Rectangle{ 64, 3328, 32, 32});
      add_prop_skull(Rectangle{ 96, 3328, 32, 32});
      add_prop_skull(Rectangle{128, 3328, 32, 32});
      add_prop_skull(Rectangle{160, 3328, 32, 32});
      add_prop_skull(Rectangle{192, 3328, 32, 32});
      add_prop_skull(Rectangle{224, 3328, 32, 32});
      add_prop_skull(Rectangle{256, 3328, 32, 32});
      add_prop_skull(Rectangle{288, 3328, 32, 32});
      add_prop_skull(Rectangle{  0, 3360, 32, 32});
      add_prop_skull(Rectangle{ 32, 3360, 32, 32});
      add_prop_skull(Rectangle{ 64, 3360, 32, 32});
      add_prop_skull(Rectangle{ 96, 3360, 32, 32});
      add_prop_skull(Rectangle{128, 3360, 32, 32});
      add_prop_skull(Rectangle{160, 3360, 32, 32});
      add_prop_skull(Rectangle{192, 3360, 32, 32});
      add_prop_skull(Rectangle{224, 3360, 32, 32});
      add_prop_skull(Rectangle{256, 3360, 32, 32});
      add_prop_skull(Rectangle{288, 3360, 32, 32});
      add_prop_skull(Rectangle{  0, 3392, 32, 32});
      add_prop_skull(Rectangle{ 32, 3392, 32, 32});
      add_prop_skull(Rectangle{ 64, 3392, 32, 32});
      add_prop_skull(Rectangle{ 96, 3392, 32, 32});
      add_prop_skull(Rectangle{128, 3392, 32, 32});
      add_prop_skull(Rectangle{160, 3392, 32, 32});
      add_prop_skull(Rectangle{192, 3392, 32, 32});
      add_prop_skull(Rectangle{224, 3392, 32, 32});
      add_prop_skull(Rectangle{256, 3392, 32, 32});
      add_prop_skull(Rectangle{288, 3392, 32, 32});
    }
    // Skulls

    // Pillar
    {
      add_prop_pillar(Rectangle{   928, 3232, 32, 64});
      add_prop_pillar(Rectangle{   960, 3232, 32, 64});
      add_prop_pillar(Rectangle{   992, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1024, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1056, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1088, 3232, 32, 64});
      add_prop_pillar(Rectangle{  1120, 3232, 32, 64});
      add_prop_pillar(Rectangle{   928, 3296, 32, 64});
      add_prop_pillar(Rectangle{   960, 3296, 32, 64});
      add_prop_pillar(Rectangle{   992, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1024, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1056, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1088, 3296, 32, 64});
      add_prop_pillar(Rectangle{  1120, 3296, 32, 64});
      add_prop_pillar(Rectangle{   928, 3360, 32, 32});
      add_prop_pillar(Rectangle{   960, 3360, 32, 32});
      add_prop_pillar(Rectangle{   992, 3360, 32, 32});
      add_prop_pillar(Rectangle{  1024, 3360, 32, 32});
      add_prop_pillar(Rectangle{  1056, 3360, 32, 32});
      add_prop_pillar(Rectangle{   928, 3392, 32, 32});
      add_prop_pillar(Rectangle{   960, 3392, 32, 32});
      add_prop_pillar(Rectangle{   992, 3392, 32, 32});
      add_prop_pillar(Rectangle{  1024, 3392, 32, 32});
      add_prop_pillar(Rectangle{  1056, 3392, 32, 32});
    } 
    // Pillar 

    // Lamps
    {
      add_prop_lamp(Rectangle{  576, 3072, 64, 80});
      add_prop_lamp(Rectangle{  640, 3072, 64, 80});
      add_prop_lamp(Rectangle{  704, 3072, 64, 80});
      add_prop_lamp(Rectangle{  768, 3072, 64, 80});
      add_prop_lamp(Rectangle{  832, 3072, 64, 80});
      add_prop_lamp(Rectangle{  576, 3168, 64, 80});
      add_prop_lamp(Rectangle{  640, 3168, 64, 80});
      add_prop_lamp(Rectangle{  704, 3168, 64, 80});
      add_prop_lamp(Rectangle{  768, 3168, 64, 80});
      add_prop_lamp(Rectangle{  832, 3168, 64, 80});
      add_prop_lamp(Rectangle{  576, 3264, 64, 80});
      add_prop_lamp(Rectangle{  640, 3264, 64, 80});
      add_prop_lamp(Rectangle{  704, 3264, 64, 80});
      add_prop_lamp(Rectangle{  768, 3264, 64, 80});
      add_prop_lamp(Rectangle{  832, 3264, 64, 80});
      add_prop_lamp(Rectangle{  576, 3360, 64, 80});
      add_prop_lamp(Rectangle{  640, 3360, 64, 80});
      add_prop_lamp(Rectangle{  704, 3360, 64, 80});
      add_prop_lamp(Rectangle{  768, 3360, 64, 80});
      add_prop_lamp(Rectangle{  832, 3360, 64, 80});
    }
    // Lamps

    // Fence
    {
      add_prop_fence(Rectangle{ 32, 2528, 32, 32});
      add_prop_fence(Rectangle{ 64, 2528, 32, 64});
      add_prop_fence(Rectangle{ 96, 2528, 32, 32});
      add_prop_fence(Rectangle{ 32, 2560, 32, 64});
      add_prop_fence(Rectangle{ 96, 2560, 32, 64});
      add_prop_fence(Rectangle{  0, 2560, 32, 64});
      add_prop_fence(Rectangle{128, 2560, 32, 64});
      add_prop_fence(Rectangle{  0, 2624, 32, 32});
      add_prop_fence(Rectangle{128, 2624, 32, 32});
      add_prop_fence(Rectangle{  0, 2656, 32, 64});
      add_prop_fence(Rectangle{ 32, 2656, 32, 64});
      add_prop_fence(Rectangle{ 32, 2720, 32, 32});
      add_prop_fence(Rectangle{ 64, 2688, 32, 64});
      add_prop_fence(Rectangle{ 96, 2656, 32, 64});
      add_prop_fence(Rectangle{ 96, 2720, 32, 32});
      add_prop_fence(Rectangle{128, 2656, 32, 64});
      add_prop_fence(Rectangle{128, 2624, 32, 32});
      add_prop_fence(Rectangle{128, 2528, 32, 32});
      add_prop_fence(Rectangle{160, 2528, 32, 32});
      add_prop_fence(Rectangle{128, 2496, 32, 32});
      add_prop_fence(Rectangle{160, 2496, 32, 32});
      add_prop_fence(Rectangle{192, 2528, 32, 32});
      add_prop_fence(Rectangle{224, 2528, 32, 32});
      add_prop_fence(Rectangle{256, 2528, 32, 32});
      add_prop_fence(Rectangle{256, 2560, 32, 32});
      add_prop_fence(Rectangle{288, 2560, 32, 32});
      add_prop_fence(Rectangle{288, 2592, 32, 32});
      add_prop_fence(Rectangle{288, 2624, 32, 32});
      add_prop_fence(Rectangle{256, 2624, 32, 32});
      add_prop_fence(Rectangle{256, 2656, 32, 32});
      add_prop_fence(Rectangle{224, 2656, 32, 32});
      add_prop_fence(Rectangle{192, 2656, 32, 32});
      add_prop_fence(Rectangle{192, 2624, 32, 32});
      add_prop_fence(Rectangle{160, 2624, 32, 32});
      add_prop_fence(Rectangle{160, 2592, 32, 32});
      add_prop_fence(Rectangle{160, 2560, 32, 32});
      add_prop_fence(Rectangle{192, 2560, 32, 32});
      add_prop_fence(Rectangle{352, 2528, 32, 32});
      add_prop_fence(Rectangle{384, 2528, 32, 32});
      add_prop_fence(Rectangle{416, 2528, 32, 32});
      add_prop_fence(Rectangle{416, 2560, 32, 32});
      add_prop_fence(Rectangle{448, 2560, 32, 32});
      add_prop_fence(Rectangle{448, 2592, 32, 32});
      add_prop_fence(Rectangle{448, 2624, 32, 32});
      add_prop_fence(Rectangle{416, 2624, 32, 32});
      add_prop_fence(Rectangle{416, 2656, 32, 32});
      add_prop_fence(Rectangle{384, 2656, 32, 32});
      add_prop_fence(Rectangle{352, 2656, 32, 32});
      add_prop_fence(Rectangle{352, 2624, 32, 32});
      add_prop_fence(Rectangle{320, 2624, 32, 32});
      add_prop_fence(Rectangle{320, 2592, 32, 32});
      add_prop_fence(Rectangle{320, 2560, 32, 32});
      add_prop_fence(Rectangle{152, 2560, 32, 64});
      add_prop_fence(Rectangle{160, 2688, 128,96});
      add_prop_fence(Rectangle{288, 2688, 128,96});
      add_prop_fence(Rectangle{ 48, 3024, 64, 64});
      add_prop_fence(Rectangle{192, 3024, 96, 64});
      add_prop_fence(Rectangle{368, 3024, 64, 64});
      add_prop_fence(Rectangle{528, 3008, 64, 64});
      add_prop_fence(Rectangle{672, 3008, 64, 64});
    }
    // Fence 

    // Detail
    {
      add_prop_detail(Rectangle{ 480, 2592, 32, 32});
      add_prop_detail(Rectangle{ 512, 2592, 32, 32});
      add_prop_detail(Rectangle{ 544, 2592, 32, 32});
      add_prop_detail(Rectangle{ 576, 2592, 32, 32});
      add_prop_detail(Rectangle{ 608, 2592, 32, 32});
      add_prop_detail(Rectangle{ 480, 2624, 32, 32});
      add_prop_detail(Rectangle{ 512, 2624, 32, 32});
      add_prop_detail(Rectangle{ 544, 2624, 32, 64});
      add_prop_detail(Rectangle{ 576, 2640, 32, 48});
      add_prop_detail(Rectangle{ 480, 2656, 32, 32});
      add_prop_detail(Rectangle{ 624, 2640, 64, 48});
      add_prop_detail(Rectangle{ 544, 2688, 32, 64});
      add_prop_detail(Rectangle{ 576, 2688, 32, 64});
      add_prop_detail(Rectangle{ 624, 2704, 48, 48});
      add_prop_detail(Rectangle{1152, 3296,400, 64}, 1.f, (Color {255, 255, 255, 128}));
    }
    // Detail 

    // Candle
    {
      add_prop_candle(Rectangle{ 320, 3104, 32, 32});
      add_prop_candle(Rectangle{ 352, 3104, 32, 32});
      add_prop_candle(Rectangle{ 384, 3104, 32, 32});
      add_prop_candle(Rectangle{ 416, 3104, 32, 32});
      add_prop_candle(Rectangle{ 448, 3104, 32, 32});
      add_prop_candle(Rectangle{ 320, 3136, 32, 32});
      add_prop_candle(Rectangle{ 352, 3136, 32, 32});
      add_prop_candle(Rectangle{ 384, 3136, 32, 32});
      add_prop_candle(Rectangle{ 416, 3136, 32, 32});
      add_prop_candle(Rectangle{ 320, 3168, 32, 32});
      add_prop_candle(Rectangle{ 352, 3168, 32, 32});
      add_prop_candle(Rectangle{ 384, 3168, 32, 32});
      add_prop_candle(Rectangle{ 416, 3168, 32, 32});
      add_prop_candle(Rectangle{ 320, 3200, 32, 32});
      add_prop_candle(Rectangle{ 352, 3200, 32, 32});
      add_prop_candle(Rectangle{ 384, 3200, 32, 32});
      add_prop_candle(Rectangle{ 416, 3200, 32, 32});
      add_prop_candle(Rectangle{ 448, 3200, 32, 32});
      add_prop_candle(Rectangle{ 320, 3232, 32, 32});
      add_prop_candle(Rectangle{ 352, 3232, 32, 32});
      add_prop_candle(Rectangle{ 384, 3232, 32, 32});
      add_prop_candle(Rectangle{ 416, 3232, 32, 32});
      add_prop_candle(Rectangle{ 320, 3264, 32, 32});
      add_prop_candle(Rectangle{ 352, 3264, 32, 32});
      add_prop_candle(Rectangle{ 384, 3264, 32, 32});
      add_prop_candle(Rectangle{ 416, 3264, 32, 32});
      add_prop_candle(Rectangle{ 448, 3264, 32, 32});
    }
    // Candle 

    // Buildings
    {
      add_prop_building(Rectangle{   0, 2800, 160, 208});
      add_prop_building(Rectangle{ 160, 2800, 160, 208});
      add_prop_building(Rectangle{ 320, 2800, 160, 208});
      add_prop_building(Rectangle{ 480, 2784, 160, 224});
      add_prop_building(Rectangle{ 640, 2800, 128, 208});
      add_prop_building(Rectangle{ 768, 2800, 160, 208});
      add_prop_building(Rectangle{ 928, 2784, 160, 224});
      add_prop_building(Rectangle{1088, 2848, 128, 160});
      add_prop_building(Rectangle{ 928, 3024, 160, 208});
      add_prop_building(Rectangle{1088, 3024, 160, 208});
    }
    // Buildings

    // Sprites
    {
      add_prop_sprite(SHEET_ID_ENVIRONMENTAL_PARTICLES);
      add_prop_sprite(SHEET_ID_LIGHT_INSECTS);
      add_prop_sprite(SHEET_ID_CANDLE_FX);
      add_prop_sprite(SHEET_ID_GENERIC_LIGHT);
    }
    // Sprites
  }

  SetTextureFilter(state->textures.at(TEX_ID_ASSET_ATLAS), TEXTURE_FILTER_POINT);
  SetTextureFilter(state->textures.at(TEX_ID_GAME_START_LOADING_SCREEN), TEXTURE_FILTER_ANISOTROPIC_16X);
  SetTextureFilter(state->textures.at(TEX_ID_WORLDMAP_WO_CLOUDS), TEXTURE_FILTER_ANISOTROPIC_16X);

  return true;
}

const atlas_texture* get_atlas_texture_by_enum(atlas_texture_id _id) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED){
    IWARN("resource::get_atlas_texture_by_enum()::Texture type out of bound");
    return nullptr;
  }
  return __builtin_addressof(state->atlas_textures.at(_id));
}
const Texture2D* get_texture_by_enum(texture_id _id) {
  if (_id >= TEX_ID_MAX or _id <= TEX_ID_UNSPECIFIED){
    IWARN("resource::get_texture_by_enum()::Texture type out of bound");
    return nullptr;
  }
  return __builtin_addressof(state->textures.at(_id));
}
const Image* get_image_by_enum(image_type type) {
  if (type >= IMAGE_TYPE_MAX or type <= IMAGE_TYPE_UNSPECIFIED){
    IWARN("resource::get_image_by_enum()::Image type out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->images.at(type));
}
const spritesheet* get_spritesheet_by_enum(spritesheet_id type) {
  if (type >= SHEET_ID_SPRITESHEET_TYPE_MAX or type <= SHEET_ID_SPRITESHEET_UNSPECIFIED){
    IWARN("resource::get_spritesheet_by_enum()::Spritesheet type out of bound");
    return nullptr;
  }
  return __builtin_addressof(state->sprites.at(type));
}
const tilesheet* get_tilesheet_by_enum(const tilesheet_type type) {
  if (type >= TILESHEET_TYPE_MAX or type <= TILESHEET_TYPE_UNSPECIFIED){
    IWARN("resource::get_tilesheet_by_enum()::Tilesheet type out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->tilesheets.at(type));
}
const char *rs_path(const char *filename) {
  return TextFormat("%s%s", RESOURCE_PATH, filename);
}
/**
 * @brief "%s%s%s", RESOURCE_PATH, MAP_PATH, filename
 */
const char *map_layer_path(const char *filename) {
  return TextFormat("%s%s%s", RESOURCE_PATH, MAP_LAYER_PATH, filename);
}
void load_texture_pak([[__maybe_unused__]] pak_file_id pak_id, [[__maybe_unused__]] i32 file_id, [[__maybe_unused__]] bool resize, [[__maybe_unused__]] 
  Vector2 new_size, [[__maybe_unused__]] texture_id _id
) {
  #if USE_PAK_FORMAT
  if (_id >= TEX_ID_MAX or _id <= TEX_ID_UNSPECIFIED) { 
    IWARN("resource::load_texture_pak()::texture type out of bound");
    return;
  }
  Texture2D tex = ZERO_TEXTURE;

  const file_buffer * const file = get_asset_file_buffer(pak_id, file_id);
  if (not file or file == nullptr or not file->is_success) {
    IERROR("resource::load_texture_pak()::File id %d does not exist", file_id);
    return;
  }
  Image img = LoadImageFromMemory(file->file_extension.c_str(), reinterpret_cast<const u8*>(file->content.data()), file->content.size());
  if (resize) {
    ImageResize(&img, new_size.x, new_size.y);
  }
  tex = LoadTextureFromImage(img);
  
  state->textures.at(_id) = tex;
  #endif
}
bool load_image_pak(
  [[__maybe_unused__]] pak_file_id pak_id, [[__maybe_unused__]] i32 file_id, [[__maybe_unused__]] bool resize, [[__maybe_unused__]] Vector2 new_size, [[__maybe_unused__]] image_type type
) {
  #if USE_PAK_FORMAT
  if (type >= IMAGE_TYPE_MAX or type <= IMAGE_TYPE_UNSPECIFIED) { 
    IWARN("resource::load_image_pak()::Image type out of bound");
    return false;
  }
  Image img = ZERO_IMAGE;

  const file_buffer * const file = get_asset_file_buffer(pak_id, file_id);
  if (not file or file == nullptr or not file->is_success) {
    IERROR("resource::load_image_pak()::File:%d does not exist", file_id);
    return false;
  }
  img = LoadImageFromMemory(file->file_extension.c_str(), reinterpret_cast<const u8 *>(file->content.size()), file->content.size());
  if (resize) {
    ImageResize(&img, new_size.x, new_size.y);
  }
  
  state->images.at(type) = img;
  return true;
  #else
    return false;
  #endif
}
void load_texture_disk(
  [[__maybe_unused__]] const char * _path, [[__maybe_unused__]] bool resize, [[__maybe_unused__]] Vector2 new_size, [[__maybe_unused__]] texture_id _id
) {
  #if not USE_PAK_FORMAT
  if (_id >= TEX_ID_MAX or _id <= TEX_ID_UNSPECIFIED) { 
    IWARN("resource::load_texture_disk()::texture type out of bound");
    return;
  }
  Texture2D tex = ZERO_TEXTURE;

  const char *path = rs_path(_path);
  if (not path or path == nullptr) {
    IERROR("resource::load_texture_disk()::Path is invalid");
    return;
  }
  if (resize) {
    Image img = LoadImage(path);
    ImageResize(&img, new_size.x, new_size.y);
    tex = LoadTextureFromImage(img);
  } else {
    tex = LoadTexture(path);
  }

  state->textures.at(_id) = tex;
  #endif
}
bool load_image_disk(
  [[__maybe_unused__]] const char * _path, [[__maybe_unused__]] bool resize, [[__maybe_unused__]] Vector2 new_size, [[__maybe_unused__]] image_type type
) {
  #if not USE_PAK_FORMAT
  if (type >= IMAGE_TYPE_MAX or type <= IMAGE_TYPE_UNSPECIFIED) { 
    IWARN("resource::load_image_disk()::Image type out of bound");
    return false;
  }
  Image img = ZERO_IMAGE;

  const char *path = rs_path(_path);
  if (not path or path == nullptr) {
    IERROR("resource::load_image_disk()::Path is invalid");
    return false;
  }
  img = LoadImage(path);
  if (resize) {
    ImageResize(&img, new_size.x, new_size.y);
  }

  state->images.at(type) = img;
  return true;
  #else
  return false;
  #endif
}

void load_spritesheet(texture_id _source_tex, spritesheet_id handle_id, Vector2 offset, i32 _fps, i32 _frame_width, i32 _frame_height, i32 _total_row, i32 _total_col) {
  if ((handle_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or handle_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) or (_source_tex >= TEX_ID_MAX or _source_tex <= TEX_ID_UNSPECIFIED)) {
    IWARN("resource::load_spritesheet()::Out of bound ID recieved");
    return;
  }
  spritesheet _sheet = spritesheet();

  _sheet.sheet_id = handle_id;
  _sheet.tex_id = _source_tex;
  _sheet.tex_handle = __builtin_addressof(state->textures.at(_source_tex));
  _sheet.offset = offset;
  _sheet.is_started = false;
  _sheet.row_total = _total_row;
  _sheet.col_total = _total_col;
  _sheet.frame_total = _total_col * _total_row;
  _sheet.tint = WHITE;
  _sheet.current_col = 0;
  _sheet.current_row = 0;
  _sheet.current_frame = 0;
  _sheet.current_frame_rect = Rectangle { 0.f, 0.f, static_cast<f32>(_frame_width), static_cast<f32>(_frame_height) };
  _sheet.coord = Rectangle { 0.f, 0.f, static_cast<f32>(_frame_width), static_cast<f32>(_frame_height)};
  _sheet.fps = static_cast<f32>(_fps);

  state->sprites.at(handle_id) = _sheet;
}
void load_tilesheet(tilesheet_type sheet_id, atlas_texture_id _atlas_tex_id, i32 _tile_count_x, i32 _tile_count_y, i32 _tile_size) {
  if (static_cast<i32>(sheet_id) >= TILESHEET_TYPE_MAX or sheet_id <= TILESHEET_TYPE_UNSPECIFIED) {
    IWARN("resource::load_tilesheet()::Sheet type out of bound");
    return;
  }
  if (_atlas_tex_id >= ATLAS_TEX_ID_MAX or _atlas_tex_id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("resource::load_tilesheet()::texture type out of bound");
  }
  tilesheet * const _tilesheet = __builtin_addressof(state->tilesheets.at(sheet_id));

  _tilesheet->atlas_source = state->atlas_textures.at(_atlas_tex_id);
  _tilesheet->sheet_id = sheet_id;
  _tilesheet->atlas_handle = &state->textures.at(ATLAS_TEXTURE_ID);
  _tilesheet->tile_count_x = _tile_count_x;
  _tilesheet->tile_count_y = _tile_count_y;
  _tilesheet->tile_count = _tilesheet->tile_count_x * _tilesheet->tile_count_y;
  _tilesheet->tile_size = _tile_size;

  for (i32 itr_000 = 0; itr_000 < _tilesheet->tile_count; ++itr_000) {
    i32 x = itr_000 % _tilesheet->tile_count_x;
    i32 y = itr_000 / _tilesheet->tile_count_x;
    i32 x_symbol = TILEMAP_TILE_START_SYMBOL + x;
    i32 y_symbol = TILEMAP_TILE_START_SYMBOL + y;

    _tilesheet->tile_symbols[x][y] = tile_symbol(x_symbol, y_symbol);
  }
}
void load_texture_from_atlas(atlas_texture_id _id, Rectangle texture_area) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) { 
    IWARN("resource::load_texture()::texture type out of bound");
    return;
  }
  atlas_texture tex = atlas_texture();
  tex.source = texture_area;
  tex.atlas_handle = __builtin_addressof(state->textures.at(ATLAS_TEXTURE_ID));

  state->atlas_textures.at(_id) = tex;
  return;
}
std::vector<tilemap_prop_static> * resource_get_tilemap_props_static(tilemap_prop_types type) {
  if (not state or state == nullptr) { 
    IERROR("resource::get_tilemap_prop_static()::State is not valid");
    return nullptr;
  }

  switch (type) {
    case TILEMAP_PROP_TYPE_TREE:      return __builtin_addressof(state->tilemap_props_trees);
    case TILEMAP_PROP_TYPE_TOMBSTONE: return __builtin_addressof(state->tilemap_props_tombstones);
    case TILEMAP_PROP_TYPE_STONE:     return __builtin_addressof(state->tilemap_props_stones);
    case TILEMAP_PROP_TYPE_SPIKE:     return __builtin_addressof(state->tilemap_props_spikes);
    case TILEMAP_PROP_TYPE_SKULL:     return __builtin_addressof(state->tilemap_props_skulls);
    case TILEMAP_PROP_TYPE_PILLAR:    return __builtin_addressof(state->tilemap_props_pillars);
    case TILEMAP_PROP_TYPE_LAMP:      return __builtin_addressof(state->tilemap_props_lamps);
    case TILEMAP_PROP_TYPE_FENCE:     return __builtin_addressof(state->tilemap_props_fence);
    case TILEMAP_PROP_TYPE_DETAIL:    return __builtin_addressof(state->tilemap_props_details);
    case TILEMAP_PROP_TYPE_CANDLE:    return __builtin_addressof(state->tilemap_props_candles);
    case TILEMAP_PROP_TYPE_BUILDING:  return __builtin_addressof(state->tilemap_props_buildings);
    default: {
      IWARN("resource::get_tilemap_prop_static()::Unsupported type");
      return nullptr;
    }
  }

  IERROR("resource::get_tilemap_prop_static()::Function ended unexpectedly");
  return nullptr;
}
std::vector<tilemap_prop_sprite> * resource_get_tilemap_props_sprite(void) {
  if (not state or state == nullptr) { 
    IERROR("resource::get_tilemap_prop_static()::State is not valid");
    return nullptr;
  }

  return __builtin_addressof(state->tilemap_props_sprite);
}
constexpr void add_prop(texture_id source_tex, tilemap_prop_types type, Rectangle source, f32 scale, Color tint) {
  if (source_tex >= TEX_ID_MAX or source_tex <= TEX_ID_UNSPECIFIED) {
    IWARN("resource::add_prop()::Provided texture id out of bound");
    return;
  }
  const Texture2D * const tex = get_texture_by_enum(source_tex);
  if (not tex or tex == nullptr) {
    IERROR("resource::add_prop()::Invalid texture");
    return;
  }
  if (tex->width < source.x + source.width or tex->height < source.y + source.height) {
    IWARN("resource::add_prop()::Provided prop dimentions out of bound");
    return;
  }
  tilemap_prop_static prop = tilemap_prop_static();

  prop.prop_id = state->next_prop_id++;
  prop.tex_id = source_tex;
  prop.prop_type = type;
  
  prop.source = source;
  prop.dest = Rectangle { 0.f, 0.f, source.width * scale, source.height * scale };
  prop.scale = scale;
  prop.tint = tint;

  prop.is_initialized = true;

  switch (type) {
    case TILEMAP_PROP_TYPE_TREE:      { state->tilemap_props_trees     .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_TOMBSTONE: { state->tilemap_props_tombstones.push_back(prop); return; }
    case TILEMAP_PROP_TYPE_STONE:     { state->tilemap_props_stones    .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_SPIKE:     { state->tilemap_props_spikes    .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_SKULL:     { state->tilemap_props_skulls    .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_PILLAR:    { state->tilemap_props_pillars   .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_LAMP:      { state->tilemap_props_lamps     .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_FENCE:     { state->tilemap_props_fence     .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_DETAIL:    { state->tilemap_props_details   .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_CANDLE:    { state->tilemap_props_candles   .push_back(prop); return; }
    case TILEMAP_PROP_TYPE_BUILDING:  { state->tilemap_props_buildings .push_back(prop); return; }
    default: { 
      IWARN("resource::add_prop()::Unsupported tilemap type");
      return;
    }
  }
  IERROR("resource::add_prop()::Function ended unexpectedly");
  return;
}
constexpr void add_prop(tilemap_prop_types type, spritesheet_id sprite_id, f32 scale, Color tint) {
  if (sprite_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or sprite_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("resource::add_prop()::Provided sprite id out of bound");
    return;
  }
  tilemap_prop_sprite prop = tilemap_prop_sprite();

  prop.prop_id = state->next_prop_id++;
  prop.prop_type = type;
  prop.scale = scale;
  prop.sprite.sheet_id = sprite_id;
  prop.sprite.tint = tint;
  
  set_sprite(&prop.sprite, true, false);

  prop.is_initialized = true;

  switch (type) {
    case TILEMAP_PROP_TYPE_SPRITE: { state->tilemap_props_sprite.push_back(prop); return; }
    default: { 
      IWARN("resource::add_prop()::Unsupported tilemap sprite type");
      return;
    }
  }
  IERROR("resource::add_prop()::Function ended unexpectedly");
  return;
}
tilemap_prop_address resource_get_map_prop_by_prop_id(i32 id, tilemap_prop_types type) {
  if (not state or state == nullptr) {
    IERROR("resource::get_map_prop_by_prop_id()::State is not valid");
    return tilemap_prop_address();
  }

  switch (type) {
    case TILEMAP_PROP_TYPE_TREE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_trees.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_trees.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_trees.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_TOMBSTONE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_tombstones.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_tombstones.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_tombstones.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_STONE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_stones.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_stones.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_stones.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_SPIKE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_spikes.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_spikes.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_spikes.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_SKULL: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_skulls.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_skulls.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_skulls.at(itr_000)));
        }
      }
    }
    case TILEMAP_PROP_TYPE_PILLAR: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_pillars.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_pillars.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_pillars.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_LAMP: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_lamps.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_lamps.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_lamps.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_FENCE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_fence.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_fence.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_fence.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_DETAIL: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_details.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_details.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_details.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_CANDLE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_candles.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_candles.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_candles.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_BUILDING: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_buildings.size(); ++itr_000) {
        tilemap_prop_static& _prop = state->tilemap_props_buildings.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_buildings.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    case TILEMAP_PROP_TYPE_SPRITE: {
      for (size_t itr_000 = 0; itr_000 < state->tilemap_props_sprite.size(); ++itr_000) {
        tilemap_prop_sprite& _prop = state->tilemap_props_sprite.at(itr_000);
        if (_prop.prop_id == id) {
          return tilemap_prop_address(__builtin_addressof(state->tilemap_props_sprite.at(itr_000)));
        }
      }
      return tilemap_prop_address();
    }
    default: { 
      IWARN("resource::get_map_prop_by_prop_id()::Unsupported tilemap type");
      return tilemap_prop_address();
    }
  }
  IERROR("resource::get_map_prop_by_prop_id()::Function ended unexpectedly");
  return tilemap_prop_address();
}

#if USE_PAK_FORMAT
const file_buffer * _get_asset_file_data(pak_file_id pak_id, i32 file_id) {
  return get_asset_file_buffer(pak_id, file_id);
}
#endif
