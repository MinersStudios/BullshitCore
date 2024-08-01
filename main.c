#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/hash.h>
#include "global_macros.h"
#include "log.h"
#include "network.h"
#include "world.h"

#define MINECRAFT_VERSION "1.21"
#define PROTOCOL_VERSION 767
#define PERROR_AND_GOTO_DESTROY(s, object) { perror(s); goto destroy_ ## object; }
#define THREAD_STACK_SIZE 8388608
#define SEND(...) \
{ \
	const uintptr_t args[] = { __VA_ARGS__ }; \
	size_t data_length; \
	size_t interthread_buffer_offset = 0; \
	ret = pthread_mutex_lock(interthread_buffer_mutex); \
	if (unlikely(ret)) \
	{ \
		errno = ret; \
		goto clear_stack_receiver; \
	} \
	for (size_t i = 0; i < NUMOF(args); i += 2) \
	{ \
		data_length = (size_t)args[i + 1]; \
		memcpy(interthread_buffer + interthread_buffer_offset, (uint8_t *)args[i], data_length); \
		interthread_buffer_offset += data_length; \
	} \
	*interthread_buffer_length = interthread_buffer_offset; \
	ret = pthread_cond_signal(interthread_buffer_condition); \
	if (unlikely(ret)) \
	{ \
		errno = ret; \
		goto clear_stack_receiver; \
	} \
	ret = pthread_mutex_unlock(interthread_buffer_mutex); \
	if (unlikely(ret)) \
	{ \
		errno = ret; \
		goto clear_stack_receiver; \
	} \
}
#define PACKET_MAXSIZE 2097151
#define REGISTRY_1 "\235\f\a\030minecraft:worldgen/biome@\022minecraft:badlands\000\027minecraft:bamboo_jungle\000\027minecraft:basalt_deltas\000\017minecraft:beach\000\026minecraft:birch_forest\000\026minecraft:cherry_grove\000\024minecraft:cold_ocean\000\030minecraft:crimson_forest\000\025minecraft:dark_forest\000\031minecraft:deep_cold_ocean\000\023minecraft:deep_dark\000\033minecraft:deep_frozen_ocean\000\035minecraft:deep_lukewarm_ocean\000\024minecraft:deep_ocean\000\020minecraft:desert\000\031minecraft:dripstone_caves\000\025minecraft:end_barrens\000\027minecraft:end_highlands\000\026minecraft:end_midlands\000\031minecraft:eroded_badlands\000\027minecraft:flower_forest\000\020minecraft:forest\000\026minecraft:frozen_ocean\000\026minecraft:frozen_peaks\000\026minecraft:frozen_river\000\017minecraft:grove\000\024minecraft:ice_spikes\000\026minecraft:jagged_peaks\000\020minecraft:jungle\000\030minecraft:lukewarm_ocean\000\024minecraft:lush_caves\000\030minecraft:mangrove_swamp\000\020minecraft:meadow\000\031minecraft:mushroom_fields\000\027minecraft:nether_wastes\000\017minecraft:ocean\000!minecraft:old_growth_birch_forest\000\037minecraft:old_growth_pine_taiga\000!minecraft:old_growth_spruce_taiga\000\020minecraft:plains\000\017minecraft:river\000\021minecraft:savanna\000\031minecraft:savanna_plateau\000\033minecraft:small_end_islands\000\025minecraft:snowy_beach\000\026minecraft:snowy_plains\000\026minecraft:snowy_slopes\000\025minecraft:snowy_taiga\000\032minecraft:soul_sand_valley\000\027minecraft:sparse_jungle\000\025minecraft:stony_peaks\000\025minecraft:stony_shore\000\032minecraft:sunflower_plains\000\017minecraft:swamp\000\017minecraft:taiga\000\021minecraft:the_end\000\022minecraft:the_void\000\024minecraft:warm_ocean\000\027minecraft:warped_forest\000\032minecraft:windswept_forest\000\"minecraft:windswept_gravelly_hills\000\031minecraft:windswept_hills\000\033minecraft:windswept_savanna\000\031minecraft:wooded_badlands\000"
#define REGISTRY_2 "\340\001\a\023minecraft:chat_type\a\016minecraft:chat\000\027minecraft:emote_command\000\036minecraft:msg_command_incoming\000\036minecraft:msg_command_outgoing\000\025minecraft:say_command\000#minecraft:team_msg_command_incoming\000#minecraft:team_msg_command_outgoing\000"
#define REGISTRY_3 "\307\002\a\026minecraft:trim_pattern\022\016minecraft:bolt\000\017minecraft:coast\000\016minecraft:dune\000\rminecraft:eye\000\016minecraft:flow\000\016minecraft:host\000\020minecraft:raiser\000\rminecraft:rib\000\020minecraft:sentry\000\020minecraft:shaper\000\021minecraft:silence\000\017minecraft:snout\000\017minecraft:spire\000\016minecraft:tide\000\rminecraft:vex\000\016minecraft:ward\000\023minecraft:wayfinder\000\016minecraft:wild\000"
#define REGISTRY_4 "\322\001\a\027minecraft:trim_material\n\022minecraft:amethyst\000\020minecraft:copper\000\021minecraft:diamond\000\021minecraft:emerald\000\016minecraft:gold\000\016minecraft:iron\000\017minecraft:lapis\000\023minecraft:netherite\000\020minecraft:quartz\000\022minecraft:redstone\000"
#define REGISTRY_5 "\270\001\a\026minecraft:wolf_variant\t\017minecraft:ashen\000\017minecraft:black\000\022minecraft:chestnut\000\016minecraft:pale\000\017minecraft:rusty\000\017minecraft:snowy\000\021minecraft:spotted\000\021minecraft:striped\000\017minecraft:woods\000"
#define REGISTRY_6 "\302\a\a\032minecraft:painting_variant2\017minecraft:alban\000\017minecraft:aztec\000\020minecraft:aztec2\000\022minecraft:backyard\000\021minecraft:baroque\000\016minecraft:bomb\000\021minecraft:bouquet\000\027minecraft:burning_skull\000\016minecraft:bust\000\022minecraft:cavebird\000\022minecraft:changing\000\017minecraft:cotan\000\021minecraft:courbet\000\021minecraft:creebet\000\025minecraft:donkey_kong\000\017minecraft:earth\000\021minecraft:endboss\000\016minecraft:fern\000\022minecraft:fighters\000\021minecraft:finding\000\016minecraft:fire\000\020minecraft:graham\000\020minecraft:humble\000\017minecraft:kebab\000\021minecraft:lowmist\000\017minecraft:match\000\024minecraft:meditative\000\rminecraft:orb\000\022minecraft:owlemons\000\021minecraft:passage\000\022minecraft:pigscene\000\017minecraft:plant\000\021minecraft:pointer\000\016minecraft:pond\000\016minecraft:pool\000\026minecraft:prairie_ride\000\rminecraft:sea\000\022minecraft:skeleton\000\031minecraft:skull_and_roses\000\017minecraft:stage\000\024minecraft:sunflowers\000\020minecraft:sunset\000\017minecraft:tides\000\022minecraft:unpacked\000\016minecraft:void\000\022minecraft:wanderer\000\023minecraft:wasteland\000\017minecraft:water\000\016minecraft:wind\000\020minecraft:wither\000"
#define REGISTRY_7 "t\a\030minecraft:dimension_type\004\023minecraft:overworld\000\031minecraft:overworld_caves\000\021minecraft:the_end\000\024minecraft:the_nether\000"
#define REGISTRY_8 "\236\b\a\025minecraft:damage_type/\017minecraft:arrow\000\033minecraft:bad_respawn_point\000\020minecraft:cactus\000\022minecraft:campfire\000\022minecraft:cramming\000\027minecraft:dragon_breath\000\017minecraft:drown\000\021minecraft:dry_out\000\023minecraft:explosion\000\016minecraft:fall\000\027minecraft:falling_anvil\000\027minecraft:falling_block\000\034minecraft:falling_stalactite\000\022minecraft:fireball\000\023minecraft:fireworks\000\027minecraft:fly_into_wall\000\020minecraft:freeze\000\021minecraft:generic\000\026minecraft:generic_kill\000\023minecraft:hot_floor\000\021minecraft:in_fire\000\021minecraft:in_wall\000\030minecraft:indirect_magic\000\016minecraft:lava\000\030minecraft:lightning_bolt\000\017minecraft:magic\000\024minecraft:mob_attack\000\035minecraft:mob_attack_no_aggro\000\030minecraft:mob_projectile\000\021minecraft:on_fire\000\026minecraft:out_of_world\000\030minecraft:outside_border\000\027minecraft:player_attack\000\032minecraft:player_explosion\000\024minecraft:sonic_boom\000\016minecraft:spit\000\024minecraft:stalagmite\000\020minecraft:starve\000\017minecraft:sting\000\032minecraft:sweet_berry_bush\000\020minecraft:thorns\000\020minecraft:thrown\000\021minecraft:trident\000\037minecraft:unattributed_fireball\000\025minecraft:wind_charge\000\020minecraft:wither\000\026minecraft:wither_skull\000"
#define REGISTRY_9 "\214\b\a\030minecraft:banner_pattern+\016minecraft:base\000\020minecraft:border\000\020minecraft:bricks\000\020minecraft:circle\000\021minecraft:creeper\000\017minecraft:cross\000\026minecraft:curly_border\000\027minecraft:diagonal_left\000\030minecraft:diagonal_right\000\032minecraft:diagonal_up_left\000\033minecraft:diagonal_up_right\000\016minecraft:flow\000\020minecraft:flower\000\017minecraft:globe\000\022minecraft:gradient\000\025minecraft:gradient_up\000\020minecraft:guster\000\031minecraft:half_horizontal\000 minecraft:half_horizontal_bottom\000\027minecraft:half_vertical\000\035minecraft:half_vertical_right\000\020minecraft:mojang\000\020minecraft:piglin\000\021minecraft:rhombus\000\017minecraft:skull\000\027minecraft:small_stripes\000\034minecraft:square_bottom_left\000\035minecraft:square_bottom_right\000\031minecraft:square_top_left\000\032minecraft:square_top_right\000\030minecraft:straight_cross\000\027minecraft:stripe_bottom\000\027minecraft:stripe_center\000\031minecraft:stripe_downleft\000\032minecraft:stripe_downright\000\025minecraft:stripe_left\000\027minecraft:stripe_middle\000\026minecraft:stripe_right\000\024minecraft:stripe_top\000\031minecraft:triangle_bottom\000\026minecraft:triangle_top\000\032minecraft:triangles_bottom\000\027minecraft:triangles_top\000"
#define REGISTRY_10 "\267\a\a\025minecraft:enchantment*\027minecraft:aqua_affinity\000\034minecraft:bane_of_arthropods\000\027minecraft:binding_curse\000\032minecraft:blast_protection\000\020minecraft:breach\000\024minecraft:channeling\000\021minecraft:density\000\027minecraft:depth_strider\000\024minecraft:efficiency\000\031minecraft:feather_falling\000\025minecraft:fire_aspect\000\031minecraft:fire_protection\000\017minecraft:flame\000\021minecraft:fortune\000\026minecraft:frost_walker\000\022minecraft:impaling\000\022minecraft:infinity\000\023minecraft:knockback\000\021minecraft:looting\000\021minecraft:loyalty\000\031minecraft:luck_of_the_sea\000\016minecraft:lure\000\021minecraft:mending\000\023minecraft:multishot\000\022minecraft:piercing\000\017minecraft:power\000\037minecraft:projectile_protection\000\024minecraft:protection\000\017minecraft:punch\000\026minecraft:quick_charge\000\025minecraft:respiration\000\021minecraft:riptide\000\023minecraft:sharpness\000\024minecraft:silk_touch\000\017minecraft:smite\000\024minecraft:soul_speed\000\027minecraft:sweeping_edge\000\025minecraft:swift_sneak\000\020minecraft:thorns\000\024minecraft:unbreaking\000\031minecraft:vanishing_curse\000\024minecraft:wind_burst\000"
#define REGISTRY_11 "\345\002\a\026minecraft:jukebox_song\023\fminecraft:11\000\fminecraft:13\000\vminecraft:5\000\020minecraft:blocks\000\rminecraft:cat\000\017minecraft:chirp\000\021minecraft:creator\000\033minecraft:creator_music_box\000\rminecraft:far\000\016minecraft:mall\000\021minecraft:mellohi\000\023minecraft:otherside\000\021minecraft:pigstep\000\023minecraft:precipice\000\017minecraft:relic\000\016minecraft:stal\000\017minecraft:strad\000\016minecraft:wait\000\016minecraft:ward\000"
#define REGISTRY_12 "\361\304\001\r\r\025minecraft:damage_type \032minecraft:bypasses_effects\001%\021minecraft:is_fall\002\t$\036minecraft:ignites_armor_stands\002\024\003\034minecraft:witch_resistant_to\004\031\026\"(\034minecraft:burns_armor_stands\001\035\035minecraft:bypasses_resistance\002\036\022 minecraft:avoids_guardian_thorns\006\031(\016\b!\001$minecraft:panic_environmental_causes\a\002\020\023\024\027\030\035\035minecraft:bypasses_wolf_armor\r\036\022\004\006\a\020\025\026\031\037%(-\030minecraft:damages_helmet\003\n\v\f\034minecraft:burn_from_stepping\002\003\023\031minecraft:bypasses_shield\024\035\025\004\006\017\021-\005%\t\020$\031\026\036\022\"\037\n\f\023minecraft:no_impact\001\006\025minecraft:is_freezing\001\020\037minecraft:bypasses_enchantments\001\"\027minecraft:is_projectile\b\000*\034+\r.),&minecraft:always_most_significant_fall\001\036\037minecraft:can_break_armor_stand\002 !\030minecraft:bypasses_armor\022\035\025\004\006\017\021-\005%\t\020$\031\026\036\022\"\037\"minecraft:bypasses_invulnerability\002\036\022\022minecraft:no_anger\001\033\026minecraft:no_knockback\033\b!\001\024\030\035\027\023\025\004\006%\002\t\017\036\021\031-\005\a'\020$\037\022\003\025minecraft:is_drowning\001\006$minecraft:always_triggers_silverfish\001\031\026minecraft:panic_causes\032\002\020\023\024\027\030\035\000\005\b\r\016\026\031\032\034 !\"&)*+,-.$minecraft:always_hurts_ender_dragons\004\016\b!\001\021minecraft:is_fire\a\024\003\035\027\023+\r\032minecraft:wither_immune_to\001\006\026minecraft:is_explosion\004\016\b!\001#minecraft:always_kills_armor_stands\005\000*\r.,\026minecraft:is_lightning\001\030\032minecraft:is_player_attack\001 \025minecraft:entity_type\"!minecraft:axolotl_always_hostiles\003\0332\035$minecraft:freeze_immune_entity_types\004fQ`w\032minecraft:immune_to_oozing\001]\020minecraft:undead\016[fx\\\v}|~\177{\0336wL\021minecraft:illager\004#7Pr)minecraft:sensitive_to_bane_of_arthropods\005\a\"Zd\020\023minecraft:frog_food\002]C!minecraft:redirectable_projectile\003>u\r\034minecraft:fall_damage_immune\0209`X\000\006\a\b\017\023-LCHKw\f\021minecraft:raiders\006#PUr7v#minecraft:powder_snow_walkable_mobs\004T\"Z*\023minecraft:arthropod\005\a\"Zd\020#minecraft:inverted_healing_and_harm\016[fx\\\v}|~\177{\0336wL\034minecraft:immune_to_infested\001Z\036minecraft:axolotl_hunt_targets\anSV\024e0h\034minecraft:sensitive_to_smite\016[fx\\\v}|~\177{\0336wL\021minecraft:zombies\a}|~\177{\0336\036minecraft:dismounts_underwater\f\016\023\0315AGMUdgl}\034minecraft:beehive_inhabitors\001\a\034minecraft:impact_projectiles\f\004c)a>^\034m\032yu\r\"minecraft:not_scary_for_pufferfish\vo2\035\024SVn\030e0h\037minecraft:non_controlling_rider\002]C\023minecraft:skeletons\005[fx\\\v\033minecraft:can_turn_in_boats\001\f\020minecraft:arrows\002\004c!minecraft:can_breathe_under_water\033[fx\\\v}|~\177{\0336wL\005+2\035o0\024SVenh\003\"minecraft:freeze_hurts_extra_types\003g\bC\030minecraft:wither_friends\016[fx\\\v}|~\177{\0336wL#minecraft:no_anger_from_wind_charge\t\f[\vf|6d\020]\021minecraft:aquatic\fo\0052\035\024SVn\030e0h\031minecraft:illager_friends\004#7Pr\"minecraft:ignores_poison_and_regen\016[fx\\\v}|~\177{\0336wL\036minecraft:deflects_projectiles\001\f\037minecraft:sensitive_to_impaling\fo\0052\035\024SVn\030e0h\030minecraft:worldgen/biomeF'minecraft:has_structure/nether_fortress\005\"0\a:\002'minecraft:has_structure/ocean_ruin_warm\003\0359\f\037minecraft:water_on_map_outlines\r\v\t\r\f\026#\006\0359(\0305\037\022minecraft:is_river\002(\030!minecraft:has_structure/mineshaft1\v\t\r\f\026#\006\0359(\030\003, \027\0332.\005=;<6/%&\001\0341\025\024\004$\b\0313!\032>\016)-'45\037*\017\036%minecraft:has_structure/jungle_temple\002\001\034&minecraft:has_structure/trial_chambers4!\v\026\t\006\r#\f\035935\037.-,<\031=/;6' \003\025&\024\004\b*)\034\000\016?\0332\030(\032%4$1\001\023>\005\027\017\036&minecraft:has_structure/village_desert\001\016&minecraft:more_frequent_drowned_spawns\002(\030\023minecraft:is_jungle\003\001\0341\033minecraft:spawns_snow_foxes\n-\032\026/\030,\027\033.\031\022minecraft:is_ocean\t\v\t\r\f\026#\006\0359\023minecraft:is_forest\006\025\024\004$\b\031'minecraft:has_structure/village_savanna\001)\026minecraft:is_overworld5!\v\026\t\006\r#\f\035935\037.-,<\031=/;6' \003\025&\024\004\b*)\034\000\016?\0332\030(\032%4$1\001\023>\005\027\017\036\n\037minecraft:without_patrol_spawns\001!%minecraft:has_structure/village_taiga\00163minecraft:allows_tropical_fish_spawns_at_any_height\001\036/minecraft:polar_bears_spawn_on_alternate_blocks\002\026\v\036minecraft:has_closer_water_fog\0025\037&minecraft:has_structure/village_plains\002' 'minecraft:has_structure/buried_treasure\002\003,'minecraft:has_structure/ocean_ruin_cold\006\026\006#\v\t\r'minecraft:produces_corals_from_bonemeal\0019\"minecraft:has_structure/stronghold5!\v\026\t\006\r#\f\035935\037.-,<\031=/;6' \003\025&\024\004\b*)\034\000\016?\0332\030(\032%4$1\001\023>\005\027\017\036\n\032minecraft:snow_golem_melts\f\000\002\a\016\023\")*0:>?,minecraft:has_structure/ruined_portal_desert\001\016&minecraft:has_structure/mineshaft_mesa\003\000\023?)minecraft:without_wandering_trader_spawns\0018$minecraft:has_structure/ancient_city\001\n+minecraft:has_structure/ruined_portal_swamp\0025\037!minecraft:has_structure/shipwreck\t\v\t\r\f\026#\006\0359\024minecraft:is_savanna\003)*>!minecraft:has_structure/swamp_hut\0015#minecraft:spawns_cold_variant_frogs\021-\032\027\033.\026\v\031\n\030/,7\021\022+\020\035minecraft:has_structure/igloo\003/-.)minecraft:has_structure/shipwreck_beached\002\003,#minecraft:has_structure/trail_ruins\0066/%&$\034\021minecraft:is_hill\003=;<,minecraft:has_structure/ruined_portal_nether\005\"0\a:\002\020minecraft:is_end\0057\021\022+\020\036minecraft:stronghold_biased_to#'4-\032\016\025\024\004\b$%&6/)*=<;>\0341\001\000\023? \031.\027\0332!\017\036\037minecraft:without_zombie_sieges\001!\022minecraft:is_beach\002\003,.minecraft:has_structure/ruined_portal_standard\026\003,(\0306/%&\025\024\004$\b\031!\032\017\036)-'4(minecraft:has_structure/pillager_outpost\f\016')-6 \027\0332.\005\031%minecraft:allows_surface_slime_spawns\0025\037\022minecraft:is_taiga\0046/%&#minecraft:spawns_warm_variant_frogs\021\0169\001\0341)*>\"0\a:\002\000\023?\037-minecraft:required_ocean_monument_surrounding\v\v\t\r\f\026#\006\0359(\030\034minecraft:mineshaft_blocking\001\n\025minecraft:is_mountain\006 \027\0332.\005%minecraft:has_structure/village_snowy\001-&minecraft:has_structure/ocean_monument\004\v\t\r\f\025minecraft:is_badlands\003\000\023?\035minecraft:spawns_gold_rabbits\001\016,minecraft:has_structure/ruined_portal_jungle\003\001\0341\036minecraft:spawns_white_rabbits\n-\032\026/\030,\027\033.\031 minecraft:plays_underwater_music\v\v\t\r\f\026#\006\0359(\030&minecraft:has_structure/desert_pyramid\001\016%minecraft:has_structure/nether_fossil\0010+minecraft:has_structure/ruined_portal_ocean\t\v\t\r\f\026#\006\0359 minecraft:has_structure/end_city\002\021\022\027minecraft:is_deep_ocean\004\v\t\r\f\023minecraft:is_nether\005\"0\a:\002'minecraft:has_structure/bastion_remnant\004\a\"0:%minecraft:reduce_water_ambient_spawns\002(\030(minecraft:has_structure/woodland_mansion\001\b minecraft:increased_fire_burnout\b\001!\037.\027\0335\034.minecraft:has_structure/ruined_portal_mountain\017\000\023?=;<*>3 \027\0332.\005\025minecraft:enchantment\026\027minecraft:tooltip_order*\002(\037\005)\016 \"\001\017\031\006\004\030$\027\n\f\021\034\033\003\v\032\t\r\022!\024\b\035\025\036\000#%\a&\023'\020\026\034minecraft:double_trade_price\a\002(%#\016\026)\022minecraft:treasure\a\002(%#\016\026)+minecraft:prevents_decorated_pot_shattering\001!\035minecraft:on_traded_equipment#\033\v\t\003\032\036\000&\a \"\001\021\n\022$\b!'\r\031\034\f\020\024\025\023\017\037\005\027\035\030\006\004\023minecraft:tradeable'\033\v\t\003\032\036\000&\a \"\001\021\n\022$\b!'\r\031\034\f\020\024\025\023\017\037\005\027\035\030\006\004\002(\016\026\030minecraft:on_random_loot'\033\v\t\003\032\036\000&\a \"\001\021\n\022$\b!'\r\031\034\f\020\024\025\023\017\037\005\027\035\030\006\004\002(\016\026\033minecraft:exclusive_set/bow\002\020\026\"minecraft:prevents_infested_spawns\001!)minecraft:prevents_bee_spawns_when_mining\001!\026minecraft:non_treasure#\033\v\t\003\032\036\000&\a \"\001\021\n\022$\b!'\r\031\034\f\020\024\025\023\017\037\005\027\035\030\006\004\037minecraft:exclusive_set/riptide\002\023\005\036minecraft:exclusive_set/damage\006 \"\001\017\006\004 minecraft:on_mob_spawn_equipment#\033\v\t\003\032\036\000&\a \"\001\021\n\022$\b!'\r\031\034\f\020\024\025\023\017\037\005\027\035\030\006\004\025minecraft:smelts_loot\001\n minecraft:exclusive_set/crossbow\002\027\030\035minecraft:exclusive_set/armor\004\033\003\v\032\035minecraft:exclusive_set/boots\002\016\a\035minecraft:in_enchanting_table#\033\v\t\003\032\036\000&\a \"\001\021\n\022$\b!'\r\031\034\f\020\024\025\023\017\037\005\027\035\030\006\004\036minecraft:prevents_ice_melting\001!\036minecraft:exclusive_set/mining\002\r!\017minecraft:curse\002\002(\025minecraft:cat_variant\002\030minecraft:default_spawns\n\000\001\002\003\004\005\006\a\b\t\032minecraft:full_moon_spawns\v\000\001\002\003\004\005\006\a\b\t\n\017minecraft:block\270\001 minecraft:mob_interactable_doors\023\303\001\307\004\310\004\311\004\312\004\314\004\272\006\273\006\315\004\316\004\313\004\316\a\317\a\321\a\320\a\322\a\323\a\325\a\324\a\023minecraft:campfires\002\222\006\223\006\037minecraft:soul_fire_base_blocks\002\200\002\201\002\033minecraft:infiniburn_nether\002\377\001\337\004\026minecraft:wooden_slabs\v\233\004\234\004\235\004\236\004\237\004\241\004\254\006\255\006\242\004\243\004\240\004\031minecraft:snaps_goat_horn\01620.1/453\001\360\003)+\250\a\326\002\023minecraft:coal_ores\002+,$minecraft:occludes_vibration_signals\020\202\001\203\001\204\001\205\001\206\001\207\001\210\001\211\001\212\001\213\001\214\001\215\001\216\001\217\001\220\001\221\001\027minecraft:small_flowers\016\223\001\225\001\226\001\227\001\230\001\231\001\232\001\233\001\234\001\235\001\236\001\240\001\237\001\224\001!minecraft:azalea_root_replaceable&\001\002\004\006\215\a\377\a\t\b\v\n\303\002\375\a\370\a\376\a7\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003$\373\001%\"\371\001\235\a\032minecraft:wooden_trapdoors\v\240\002\236\002\242\002\237\002\234\002\235\002\262\006\263\006\243\002\244\002\241\002\036minecraft:invalid_spawn_inside\002\317\002\333\004\035minecraft:wolves_spawnable_on\005\b\367\001\371\001\n\v\034minecraft:foxes_spawnable_on\005\b\367\001\371\001\v\n\016minecraft:wool\020\202\001\203\001\204\001\205\001\206\001\207\001\210\001\211\001\212\001\213\001\214\001\215\001\216\001\217\001\220\001\221\001\020minecraft:stairs8\260\001\334\002\335\002\336\002\311\003\313\003\266\006\267\006\314\003\315\003\312\003\316\003\306\001\325\002\307\002\301\002\300\002\324\004\246\003\232\004\327\003\326\003\330\003\334\005\335\005\336\005\337\005\340\005\341\005\342\005\343\005\344\005\345\005\346\005\347\005\350\005\351\005\322\006\332\006\335\006\201\b\205\b\211\b\215\b\266\a\267\a\270\a\271\a\307\a\310\a\311\a\306\a\302\002\217\a\223\a\230\a\016minecraft:logs(4H>P.B?J2F<N0D:L1E;M/C9K5I@Q3G=O\236\006\237\006\240\006\241\006\225\006\226\006\227\006\230\006!minecraft:trail_ruins_replaceable\001%\023minecraft:all_signs,\272\001\273\001\274\001\275\001\277\001\300\001\274\006\275\006\301\001\302\001\276\001\307\001\310\001\311\001\312\001\314\001\315\001\276\006\277\006\316\001\317\001\313\001\320\001\321\001\322\001\323\001\324\001\325\001\326\001\327\001\330\001\331\001\332\001\333\001\334\001\335\001\336\001\337\001\340\001\341\001\343\001\344\001\342\001\345\001\022minecraft:beehives\002\304\006\305\006\rminecraft:ice\004\370\001\360\003\324\005\336\004$minecraft:enchantment_power_provider\001\247\001\031minecraft:azalea_grows_on\037\t\b\v\n\303\002\375\a\370\a\376\a7\"$#\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\371\001\235\a\026minecraft:wool_carpets\020\336\003\337\003\340\003\341\003\342\003\343\003\344\003\345\003\346\003\347\003\350\003\351\003\352\003\353\003\354\003\355\003\017minecraft:crops\b\331\004\377\002\200\003\267\001\274\002\273\002\326\004\327\004\027minecraft:dragon_immune\021\320\003\037\317\002\320\002\333\004\337\002\334\004\335\004\300\006\301\006\222\001\252\001\312\006\321\002\264\002\313\006\236\b)minecraft:mangrove_roots_can_grow_through\a\376\a76\366\a\275\002\036\367\001!minecraft:features_cannot_replace\a\037\257\001\261\001\320\002\236\b\241\b\242\b\025minecraft:valid_spawn\002\b\v\035minecraft:mushroom_grow_block\004\303\002\v\242\006\231\006\026minecraft:wooden_doors\v\303\001\307\004\310\004\311\004\312\004\314\004\272\006\273\006\315\004\316\004\313\004\036minecraft:crystal_sound_blocks\002\207\a\210\a!minecraft:sniffer_egg_hatch_boost\001\370\a\030minecraft:infiniburn_end\003\377\001\337\004\037\030minecraft:standing_signs\v\272\001\273\001\274\001\275\001\277\001\300\001\274\006\275\006\301\001\302\001\276\001\026minecraft:warped_stems\004\225\006\226\006\227\006\230\006\026minecraft:emerald_ores\002\326\002\327\002\027minecraft:bamboo_blocks\0028A\027minecraft:crimson_stems\004\236\006\237\006\240\006\241\006\036minecraft:replaceable_by_trees\035URSXVTZ[YW{|}\275\002\276\002\361\003\362\003\363\003\364\003\365\003\366\003\374\a\330\004 ~\177\234\006\235\006\251\006\032minecraft:needs_stone_toolT\244\001\225\b)*a_`\244\a\226\b\250\a\251\a\275\a\271\a\255\a\246\a\273\a\267\a\253\a\247\a\272\a\266\a\252\a\245\a\274\a\270\a\254\a\276\a\315\a\311\a\305\a\277\a\313\a\307\a\303\a\300\a\314\a\310\a\304\a\301\a\312\a\306\a\302\a\356\a\240\b\261\a\260\a\257\a\256\a\265\a\264\a\263\a\262\a\336\a\337\a\340\a\341\a\342\a\343\a\344\a\345\a\346\a\347\a\350\a\351\a\352\a\353\a\354\a\355\a\326\a\327\a\331\a\330\a\332\a\333\a\335\a\334\a\316\a\317\a\321\a\320\a\322\a\323\a\325\a\324\a\031minecraft:concrete_powder\020\226\005\227\005\230\005\231\005\232\005\233\005\234\005\235\005\236\005\237\005\240\005\241\005\242\005\243\005\244\005\245\005(minecraft:lava_pool_stone_cannot_replace9\037\257\001\261\001\320\002\236\b\241\b\242\bURSXVTZ[YW4H>P.B?J2F<N0D:L1E;M/C9K5I@Q3G=O\236\006\237\006\240\006\241\006\225\006\226\006\227\006\230\006\"minecraft:inside_step_sound_blocks\006\235\a\241\a\276\002\304\002\214\a\367\a%minecraft:prevent_mob_spawning_inside\004\305\001wx\247\003\024minecraft:terracotta\021\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\023minecraft:climbable\t\304\001\275\002\204\006\245\006\246\006\247\006\250\006\361\a\362\a\025minecraft:wart_blocks\002\340\004\233\006\036minecraft:parrots_spawnable_on4\b\000URSXVTZ[YW4H>P.B?J2F<N0D:L1E;M/C9K5I@Q3G=O\236\006\237\006\240\006\241\006\225\006\226\006\227\006\230\006\027minecraft:dark_oak_logs\0044H>P\035minecraft:frog_prefer_jump_to\002\304\002\371\a\026minecraft:coral_plants\005\272\005\273\005\274\005\275\005\276\005\034minecraft:goats_spawnable_on\006\b\001\367\001\371\001\360\003%%minecraft:sculk_replaceable_world_gen7\001\002\004\006\215\a\377\a\t\b\v\n\303\002\375\a\370\a\376\a7\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\242\006\231\006\377\001\202\002\321\006\"$%\200\002\201\002\233\a\224\b\373\001\360\a\321\002\227\004c\214\b\210\b\200\b\221\b\222\b\204\b\037minecraft:ceiling_hanging_signs\v\320\001\321\001\322\001\323\001\324\001\325\001\326\001\327\001\330\001\331\001\332\001\034minecraft:beacon_base_blocks\005\310\006\333\002\265\001\243\001\244\001\034minecraft:frogs_spawnable_on\004\b\376\a67\027minecraft:shulker_boxes\021\345\004\365\004\361\004\362\004\357\004\355\004\363\004\351\004\356\004\353\004\350\004\347\004\354\004\360\004\364\004\346\004\352\004'minecraft:blocks_wind_charge_explosions\002\320\003\037\017minecraft:anvil\003\230\003\231\003\232\003\024minecraft:birch_logs\0040D:L\024minecraft:lapis_ores\002_`\032minecraft:moss_replaceable\021\001\002\004\006\215\a\377\a\362\a\361\a\t\b\v\n\303\002\375\a\370\a\376\a7\025minecraft:wall_corals\005\316\005\317\005\320\005\321\005\322\005\034minecraft:maintains_farmland\v\273\002\271\002\274\002\272\002\331\004\377\002\200\003\326\004\224\001\327\004\267\001\034minecraft:convertable_to_mud\003\t\n\375\a\rminecraft:air\003\000\331\005\332\005 minecraft:sniffer_diggable_block\b\t\b\v\n\375\a\370\a\376\a7!minecraft:lush_ground_replaceable\024\001\002\004\006\215\a\377\a\362\a\361\a\t\b\v\n\303\002\375\a\370\a\376\a7\373\001%\"\020minecraft:fences\f\376\001\302\004\304\004\277\004\300\004\301\004\260\006\261\006\305\004\306\004\303\004\306\002\022minecraft:saplings\n\027\030\031\032\033\035\364\a\365\a\036\034\016minecraft:beds\020uvrspntjolihmqgk\032minecraft:mineable/pickaxe\256\003\001\002\003\004\005\006\a\f'()*+,-_`abcde\243\001\244\001\245\001\251\001\252\001\257\001\263\001\264\001\265\001\271\001\306\001\347\001\350\001\362\001\363\001\377\001\202\002\203\002\245\002\246\002\247\002\250\002\264\002\265\002\300\002\301\002\305\002\306\002\307\002\311\002\312\002\321\002\325\002\326\002\327\002\330\002\333\002\234\003\235\003\240\003\241\003\242\003\243\003\244\003\245\003\246\003\250\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\322\003\323\003\324\003\325\003\326\003\327\003\330\003\331\003\332\003\333\003\356\003\357\003\227\004\230\004\231\004\232\004\245\004\246\004\247\004\250\004\251\004\252\004\253\004\254\004\256\004\257\004\260\004\261\004\262\004\263\004\264\004\265\004\266\004\322\004\323\004\324\004\325\004\337\004\341\004\342\004\344\004\366\004\367\004\370\004\371\004\372\004\373\004\374\004\375\004\376\004\377\004\200\005\201\005\202\005\203\005\204\005\205\005\206\005\207\005\210\005\211\005\212\005\213\005\214\005\215\005\216\005\217\005\220\005\221\005\222\005\223\005\224\005\225\005\253\005\254\005\255\005\256\005\257\005\260\005\261\005\262\005\263\005\264\005\265\005\266\005\267\005\270\005\271\005\277\005\300\005\301\005\302\005\303\005\311\005\312\005\313\005\314\005\315\005\334\005\335\005\336\005\337\005\340\005\341\005\342\005\343\005\344\005\345\005\346\005\347\005\350\005\351\005\352\005\353\005\354\005\355\005\356\005\357\005\360\005\361\005\362\005\363\005\364\005\365\005\366\005\207\006\210\006\213\006\216\006\217\006\220\006\221\006\231\006\242\006\310\006\311\006\312\006\313\006\320\006\321\006\322\006\324\006\325\006\326\006\327\006\330\006\331\006\332\006\334\006\335\006\336\006\337\006\342\006\343\006\344\006\215\a\233\a\247\a\246\a\245\a\244\a\250\a\251\a\252\a\253\a\254\a\255\a\266\a\267\a\270\a\271\a\272\a\273\a\274\a\275\a\276\a\277\a\300\a\301\a\302\a\303\a\304\a\305\a\306\a\307\a\310\a\311\a\312\a\313\a\314\a\315\a\356\a\357\a\360\a\377\a\200\b\201\b\202\b\204\b\205\b\206\b\210\b\211\b\212\b\214\b\215\b\216\b\220\b\221\b\222\b\224\b\225\b\226\b\227\b\370\001\360\003\324\005\200\001y\201\001\211\a\214\a\213\a\212\a\207\a\210\a\254\002\260\002\257\002\223\b\253\002\256\002\255\002\366\001\340\006\341\002\342\002\367\005\370\005\371\005\372\005\373\005\374\005\376\005\377\005\200\006\201\006\202\006\203\006\323\006\333\006\341\006\203\b\207\b\213\b\217\b\375\005\220\a\224\a\231\a\345\004\365\004\361\004\362\004\357\004\355\004\363\004\351\004\356\004\353\004\350\004\347\004\354\004\360\004\364\004\346\004\352\004\230\003\231\003\232\003\313\002\314\002\315\002\316\002\305\001wx\247\003\325\005\252\002\302\002\255\004\251\002\240\b\216\a\217\a\225\a\221\a\222\a\223\a\226\a\227\a\230\a\232\a\261\a\260\a\257\a\256\a\265\a\264\a\263\a\262\a\336\a\337\a\340\a\341\a\342\a\343\a\344\a\345\a\346\a\347\a\350\a\351\a\352\a\353\a\354\a\355\a\316\a\317\a\321\a\320\a\322\a\323\a\325\a\324\a\326\a\327\a\331\a\330\a\332\a\333\a\335\a\334\a\243\b\023minecraft:iron_ores\002)* minecraft:unstable_bottom_center\v\272\004\270\004\274\004\271\004\277\002\267\004\264\006\265\006\275\004\276\004\273\004\022minecraft:oak_logs\004.B?J\017minecraft:doors\024\303\001\307\004\310\004\311\004\312\004\314\004\272\006\273\006\315\004\316\004\313\004\316\a\317\a\321\a\320\a\322\a\323\a\325\a\324\a\350\001\033minecraft:enderman_holdable(\223\001\225\001\226\001\227\001\230\001\231\001\232\001\233\001\234\001\235\001\236\001\240\001\237\001\224\001\t\b\v\n\303\002\375\a\370\a\376\a7\"$%\241\001\242\001\246\001\372\001\373\001\267\002\210\002\270\002\243\006\242\006\251\006\232\006\231\006\234\006\021minecraft:banners \367\003\370\003\371\003\372\003\373\003\374\003\375\003\376\003\377\003\200\004\201\004\202\004\203\004\204\004\205\004\206\004\207\004\210\004\211\004\212\004\213\004\214\004\215\004\216\004\217\004\220\004\221\004\222\004\223\004\224\004\225\004\226\004\036minecraft:infiniburn_overworld\002\377\001\337\004\031minecraft:smelts_to_glass\002\"$\025minecraft:flower_pots#\343\002\357\002\360\002\361\002\362\002\363\002\364\002\365\002\366\002\367\002\356\002\345\002\346\002\347\002\350\002\351\002\353\002\373\002\374\002\375\002\355\002\376\002\370\002\371\002\372\002\330\005\314\006\315\006\316\006\317\006\230\b\231\b\354\002\352\002\344\002\033minecraft:piglin_repellents\005\256\001\204\002\221\006\205\002\223\006\027minecraft:wooden_fences\v\376\001\302\004\304\004\277\004\300\004\301\004\260\006\261\006\305\004\306\004\303\004#minecraft:incorrect_for_wooden_toole\252\001\312\006\310\006\313\006\311\006\265\001\263\001\264\001\326\002\327\002\333\002\243\001\227\b'(\362\001\363\001\244\001\225\b)*a_`\244\a\226\b\250\a\251\a\275\a\271\a\255\a\246\a\273\a\267\a\253\a\247\a\272\a\266\a\252\a\245\a\274\a\270\a\254\a\276\a\315\a\311\a\305\a\277\a\313\a\307\a\303\a\300\a\314\a\310\a\304\a\301\a\312\a\306\a\302\a\356\a\240\b\261\a\260\a\257\a\256\a\265\a\264\a\263\a\262\a\336\a\337\a\340\a\341\a\342\a\343\a\344\a\345\a\346\a\347\a\350\a\351\a\352\a\353\a\354\a\355\a\326\a\327\a\331\a\330\a\332\a\333\a\335\a\334\a\316\a\317\a\321\a\320\a\322\a\323\a\325\a\324\a!minecraft:incorrect_for_iron_tool\005\252\001\312\006\310\006\313\006\311\006'minecraft:enchantment_power_transmitter\030\000 !{|}~\177\255\001\256\001\367\001\275\002\276\002\321\003\365\003\366\003\343\004\331\005\332\005\333\005\234\006\235\006\251\006\374\a\034minecraft:wall_post_overrideI\253\001\204\002\364\001\332\002\272\001\273\001\274\001\275\001\277\001\300\001\274\006\275\006\301\001\302\001\276\001\307\001\310\001\311\001\312\001\314\001\315\001\276\006\277\006\316\001\317\001\313\001\367\003\370\003\371\003\372\003\373\003\374\003\375\003\376\003\377\003\200\004\201\004\202\004\203\004\204\004\205\004\206\004\207\004\210\004\211\004\212\004\213\004\214\004\215\004\216\004\217\004\220\004\221\004\222\004\223\004\224\004\225\004\226\004\234\003\235\003\351\001\352\001\353\001\354\001\355\001\357\001\256\006\257\006\360\001\361\001\356\001\347\001\337\006!minecraft:mooshrooms_spawnable_on\001\303\002\021minecraft:portals\003\207\002\317\002\333\004\035minecraft:bamboo_plantable_on\020\"$#\t\b\v\n\303\002\375\a\370\a\376\a7\327\005\326\005%&,minecraft:polar_bears_spawnable_on_alternate\001\370\001\023minecraft:cauldrons\004\313\002\314\002\315\002\316\002 minecraft:big_dripleaf_placeable\v\373\001\370\a\t\b\v\n\303\002\375\a\376\a7\270\001\031minecraft:sword_efficientWURSXVTZ[YW\027\030\031\032\033\035\364\a\365\a\036\034\223\001\225\001\226\001\227\001\230\001\231\001\232\001\233\001\234\001\235\001\236\001\240\001\237\001\224\001\331\004\377\002\200\003\267\001\274\002\273\002\326\004\327\004{|}\275\002\276\002\361\003\362\003\363\003\364\003\365\003\366\003\374\a\330\004\241\001\242\001\374\001\267\002\210\002\211\002\270\002\271\002\272\002\304\002\324\002\224\006\361\a\362\a\363\a\366\a\367\a\371\a\372\a\373\a\310\002\232\006\234\006\235\006\243\006\245\006\246\006\247\006\250\006\251\006\320\004\321\004\031minecraft:pressure_plates\017\234\003\235\003\351\001\352\001\353\001\354\001\355\001\357\001\256\006\257\006\360\001\361\001\356\001\347\001\337\006\034minecraft:dampens_vibrations \202\001\203\001\204\001\205\001\206\001\207\001\210\001\211\001\212\001\213\001\214\001\215\001\216\001\217\001\220\001\221\001\336\003\337\003\340\003\341\003\342\003\343\003\344\003\345\003\346\003\347\003\350\003\351\003\352\003\353\003\354\003\355\003\027minecraft:mangrove_logs\0045I@Q'minecraft:overworld_carver_replaceables1\001\002\004\006\215\a\377\a\t\b\v\n\303\002\375\a\370\a\376\a7\"$#\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003)*\250\a\251\a %&c\227\004\233\a\367\001\360\003\225\b\226\b#minecraft:snow_layer_can_survive_on\003\306\006\200\002\376\a\025minecraft:jungle_logs\0041E;M minecraft:dead_bush_may_place_on\035\"$#\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\t\b\v\n\303\002\375\a\370\a\376\a7\036minecraft:vibration_resonators\001\207\a\034minecraft:wall_hanging_signs\v\333\001\334\001\335\001\336\001\337\001\340\001\341\001\343\001\344\001\342\001\345\001&minecraft:snow_layer_cannot_survive_on\003\370\001\360\003\320\003\033minecraft:sculk_replaceable1\001\002\004\006\215\a\377\a\t\b\v\n\303\002\375\a\370\a\376\a7\356\003\251\003\252\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\242\006\231\006\377\001\202\002\321\006\"$%\200\002\201\002\233\a\224\b\373\001\360\a\321\002\227\004c\025minecraft:spruce_logs\004/C9K\027minecraft:wooden_stairs\v\260\001\334\002\335\002\336\002\311\003\313\003\266\006\267\006\314\003\315\003\312\003\017minecraft:signs\026\272\001\273\001\274\001\275\001\277\001\300\001\274\006\275\006\301\001\302\001\276\001\307\001\310\001\311\001\312\001\314\001\315\001\276\006\277\006\316\001\317\001\313\001\"minecraft:ancient_city_replaceable\f\377\a\214\b\210\b\216\b\212\b\215\b\213\b\217\b\200\b\221\b\222\b\211\001\036minecraft:base_stone_overworld\006\001\002\004\006\215\a\377\a(minecraft:mangrove_logs_can_grow_through\b\376\a76Y5\036\366\a\275\002\030minecraft:wooden_buttons\v\201\003\202\003\203\003\204\003\205\003\207\003\270\006\271\006\210\003\211\003\206\003\037minecraft:axolotls_spawnable_on\001\373\001#minecraft:wither_summon_base_blocks\002\200\002\201\002&minecraft:dripstone_replaceable_blocks\006\001\002\004\006\215\a\377\a\033minecraft:hoglin_repellents\004\232\006\315\006\207\002\313\006\026minecraft:stone_bricks\004\245\002\246\002\247\002\250\002\016minecraft:fire\002\255\001\256\001\026minecraft:mineable/axe\251\002f\272\002\271\002\364\a\327\005\206\006\304\006\305\006\331\004\372\a\371\a\247\001\261\002\241\001\222\006\377\002\211\006\210\002\362\a\361\a\261\001\321\004\320\004\324\002\302\006\266\001\243\006\237\003}|\212\006\276\002{\374\a\211\002\375\001\304\001\366\003\214\006\304\002\205\006\274\002\270\002\263\002\310\002\200\003\273\002\267\002\262\002\242\001\204\006\373\a\215\006\223\006\363\a\374\001\224\006\365\003\233\003\250\006\247\006\275\002\232\006\246\006\245\006\267\001\367\003\370\003\371\003\372\003\373\003\374\003\375\003\376\003\377\003\200\004\201\004\202\004\203\004\204\004\205\004\206\004\207\004\210\004\211\004\212\004\213\004\214\004\215\004\216\004\217\004\220\004\221\004\222\004\223\004\224\004\225\004\226\004\272\004\270\004\274\004\271\004\277\002\267\004\264\006\265\006\275\004\276\004\273\0044H>P.B?J2F<N0D:L1E;M/C9K5I@Q3G=O\236\006\237\006\240\006\241\006\225\006\226\006\227\006\230\006\r\016\017\020\021\023\252\006\253\006\024\025\022\027\030\031\032\033\035\365\a\036\034\272\001\273\001\274\001\275\001\277\001\300\001\274\006\275\006\301\001\302\001\276\001\307\001\310\001\311\001\312\001\314\001\315\001\276\006\277\006\316\001\317\001\313\001\201\003\202\003\203\003\204\003\205\003\207\003\270\006\271\006\210\003\211\003\206\003\303\001\307\004\310\004\311\004\312\004\314\004\272\006\273\006\315\004\316\004\313\004\376\001\302\004\304\004\277\004\300\004\301\004\260\006\261\006\305\004\306\004\303\004\351\001\352\001\353\001\354\001\355\001\357\001\256\006\257\006\360\001\361\001\356\001\233\004\234\004\235\004\236\004\237\004\241\004\254\006\255\006\242\004\243\004\240\004\260\001\334\002\335\002\336\002\311\003\313\003\266\006\267\006\314\003\315\003\312\003\240\002\236\002\242\002\237\002\234\002\235\002\262\006\263\006\243\002\244\002\241\0026\320\001\321\001\322\001\323\001\324\001\325\001\326\001\327\001\330\001\331\001\332\001\333\001\334\001\335\001\336\001\337\001\340\001\341\001\343\001\344\001\342\001\345\001\026\244\004\316\0038A\250\001\033minecraft:base_stone_nether\003\377\001\202\002\321\006\034minecraft:needs_diamond_tool\005\252\001\312\006\310\006\313\006\311\006\024minecraft:wall_signs\v\307\001\310\001\311\001\312\001\314\001\315\001\276\006\277\006\316\001\317\001\313\001\"minecraft:incorrect_for_stone_tool\021\252\001\312\006\310\006\313\006\311\006\265\001\263\001\264\001\326\002\327\002\333\002\243\001\227\b'(\362\001\363\001\017minecraft:slabs<\233\004\234\004\235\004\236\004\237\004\241\004\254\006\255\006\242\004\243\004\240\004\244\004\245\004\246\004\254\004\247\004\262\004\257\004\260\004\253\004\252\004\256\004\251\004\331\003\332\003\333\003\352\005\353\005\354\005\355\005\356\005\357\005\360\005\361\005\362\005\363\005\364\005\365\005\366\005\250\004\261\004\324\006\331\006\336\006\202\b\206\b\212\b\216\b\313\a\314\a\315\a\272\a\273\a\274\a\275\a\312\a\255\004\216\a\222\a\227\a\036minecraft:animals_spawnable_on\001\b\034minecraft:guarded_by_piglins\033\243\001\206\006\261\001\330\002\334\006\233\003\227\b\345\004\365\004\361\004\362\004\357\004\355\004\363\004\351\004\356\004\353\004\350\004\347\004\354\004\360\004\364\004\346\004\352\004'-(\031minecraft:mineable/shovel$\373\001\t\n\v\270\001\b%\303\002\"$\371\001\367\001\200\002\332\004\201\002\375\a7\376\a#&\226\005\227\005\230\005\231\005\232\005\233\005\234\005\235\005\236\005\237\005\240\005\241\005\242\005\243\005\244\005\245\005$minecraft:nether_carver_replaceables\030\001\002\004\006\215\a\377\a\377\001\202\002\321\006\t\b\v\n\303\002\375\a\370\a\376\a7\242\006\231\006\340\004\233\006\200\002\201\002 minecraft:stone_ore_replaceables\004\001\002\004\006\023minecraft:trapdoors\024\240\002\236\002\242\002\237\002\234\002\235\002\262\006\263\006\243\002\244\002\241\002\322\003\326\a\327\a\331\a\330\a\332\a\333\a\335\a\334\a\027minecraft:redstone_ores\002\362\001\363\001\025minecraft:cherry_logs\0043G=O\037minecraft:fall_damage_resetting\v\304\001\275\002\204\006\245\006\246\006\247\006\250\006\361\a\362\a\224\006z\021minecraft:buttons\r\201\003\202\003\203\003\204\003\205\003\207\003\270\006\271\006\210\003\211\003\206\003\366\001\340\006\021minecraft:flowers\032\223\001\225\001\226\001\227\001\230\001\231\001\232\001\233\001\234\001\235\001\236\001\240\001\237\001\224\001\361\003\362\003\364\003\363\003\330\004[\365\a\036W\367\a\321\004\363\a\020minecraft:corals\n\272\005\273\005\274\005\275\005\276\005\304\005\305\005\306\005\307\005\310\005'minecraft:combination_step_sound_blocks\025\336\003\337\003\340\003\341\003\342\003\343\003\344\003\345\003\346\003\347\003\350\003\351\003\352\003\353\003\354\003\355\003\366\a\367\001\235\006\234\006\251\006\036minecraft:rabbits_spawnable_on\004\b\367\001\371\001\"\020minecraft:planks\v\r\016\017\020\021\023\252\006\253\006\024\025\022\027minecraft:stone_buttons\002\366\001\340\006 minecraft:does_not_block_hoppers\002\304\006\305\006\033minecraft:soul_speed_blocks\002\200\002\201\002\017minecraft:rails\004\305\001wx\247\003\026minecraft:diamond_ores\002\263\001\264\001\036minecraft:geode_invalid_blocks\006\037 !\370\001\360\003\324\005\035minecraft:badlands_terracotta\a\356\003\251\003\255\003\252\003\267\003\265\003\261\003\033minecraft:all_hanging_signs\026\320\001\321\001\322\001\323\001\324\001\325\001\326\001\327\001\330\001\331\001\332\001\333\001\334\001\335\001\336\001\337\001\340\001\341\001\343\001\344\001\342\001\345\001 minecraft:overworld_natural_logs\b20.1/453\020minecraft:leaves\nURSXVTZ[YW$minecraft:deepslate_ore_replaceables\002\377\a\215\a\017minecraft:walls\031\341\002\342\002\367\005\370\005\371\005\372\005\373\005\374\005\376\005\377\005\200\006\201\006\202\006\203\006\323\006\333\006\341\006\203\b\207\b\213\b\217\b\375\005\220\a\224\a\231\a\026minecraft:coral_blocks\005\260\005\261\005\262\005\263\005\264\005\024minecraft:cave_vines\002\362\a\361\a\035minecraft:strider_warm_blocks\001!\025minecraft:fence_gates\v\272\004\270\004\274\004\271\004\277\002\267\004\264\006\265\006\275\004\276\004\273\004\027minecraft:bee_growables\v\331\004\377\002\200\003\267\001\274\002\273\002\326\004\327\004\224\006\361\a\362\a!minecraft:incorrect_for_gold_toole\252\001\312\006\310\006\313\006\311\006\265\001\263\001\264\001\326\002\327\002\333\002\243\001\227\b'(\362\001\363\001\244\001\225\b)*a_`\244\a\226\b\250\a\251\a\275\a\271\a\255\a\246\a\273\a\267\a\253\a\247\a\272\a\266\a\252\a\245\a\274\a\270\a\254\a\276\a\315\a\311\a\305\a\277\a\313\a\307\a\303\a\300\a\314\a\310\a\304\a\301\a\312\a\306\a\302\a\356\a\240\b\261\a\260\a\257\a\256\a\265\a\264\a\263\a\262\a\336\a\337\a\340\a\341\a\342\a\343\a\344\a\345\a\346\a\347\a\350\a\351\a\352\a\353\a\354\a\355\a\326\a\327\a\331\a\330\a\332\a\333\a\335\a\334\a\316\a\317\a\321\a\320\a\322\a\323\a\325\a\324\a minecraft:wooden_pressure_plates\v\351\001\352\001\353\001\354\001\355\001\357\001\256\006\257\006\360\001\361\001\356\001\027minecraft:wither_immune\r\320\003\037\317\002\320\002\333\004\337\002\334\004\335\004\300\006\301\006\222\001\321\003\236\b minecraft:armadillo_spawnable_on\n\b\356\003\251\003\255\003\252\003\267\003\265\003\261\003$\n\025minecraft:acacia_logs\0042F<N&minecraft:incorrect_for_netherite_tool\000\021minecraft:candles\021\345\006\346\006\347\006\350\006\351\006\352\006\353\006\354\006\355\006\356\006\357\006\360\006\361\006\362\006\363\006\364\006\365\006\026minecraft:tall_flowers\005\361\003\362\003\364\003\363\003\330\004\034minecraft:dragon_transparent\003\321\003\255\001\256\001\036minecraft:underwater_bonemeals\020~\272\005\273\005\274\005\275\005\276\005\304\005\305\005\306\005\307\005\310\005\316\005\317\005\320\005\321\005\322\005\037minecraft:stone_pressure_plates\002\347\001\337\006\025minecraft:impermeable\022^\214\002\215\002\216\002\217\002\220\002\221\002\222\002\223\002\224\002\225\002\226\002\227\002\230\002\231\002\232\002\233\002\234\a\025minecraft:copper_ores\002\250\a\251\a\016minecraft:sand\003\"$#\020minecraft:nylium\002\242\006\231\006\016minecraft:snow\003\367\001\371\001\235\a\023minecraft:gold_ores\003'-(\"minecraft:small_dripleaf_placeable\002\373\001\370\a$minecraft:incorrect_for_diamond_tool\000\030minecraft:logs_that_burn 4H>P.B?J2F<N0D:L1E;M/C9K5I@Q3G=O&minecraft:completes_find_tree_tutorial44H>P.B?J2F<N0D:L1E;M/C9K5I@Q3G=O\236\006\237\006\240\006\241\006\225\006\226\006\227\006\230\006URSXVTZ[YW\340\004\233\006&minecraft:camel_sand_step_sound_blocks\023\"$#\226\005\227\005\230\005\231\005\232\005\233\005\234\005\235\005\236\005\237\005\240\005\241\005\242\005\243\005\244\005\245\005\026minecraft:mineable/hoe\033\340\004\233\006\335\003\250\005\303\006\244\006\\]URSXVTZ[Y\236\a\237\a\370\a\366\a\240\a\242\a\241\a\243\a\367\aW\025minecraft:replaceable\030\000 !{|}~\177\255\001\256\001\367\001\275\002\276\002\321\003\365\003\366\003\343\004\331\005\332\005\333\005\234\006\235\006\251\006\374\a\016minecraft:dirt\t\t\b\v\n\303\002\375\a\370\a\376\a7\026minecraft:candle_cakes\021\366\006\367\006\370\006\371\006\372\006\373\006\374\006\375\006\376\006\377\006\200\a\201\a\202\a\203\a\204\a\205\a\206\a\031minecraft:needs_iron_tool\f\265\001\263\001\264\001\326\002\327\002\333\002\243\001\227\b'(\362\001\363\001\024minecraft:instrument\003\036minecraft:screaming_goat_horns\004\004\005\006\a\024minecraft:goat_horns\b\000\001\002\003\004\005\006\a\034minecraft:regular_goat_horns\004\000\001\002\003\017minecraft:fluid\002\016minecraft:lava\002\004\003\017minecraft:water\002\002\001\030minecraft:banner_pattern\t\035minecraft:pattern_item/piglin\001\026\032minecraft:no_item_required\"\032\033\034\035\037&#% $\"!\031\005\036'()*\a\n\t\b\003\027\023\021\024\022\001\006\016\017\002\035minecraft:pattern_item/flower\001\f\036minecraft:pattern_item/creeper\001\004\033minecraft:pattern_item/flow\001\v\034minecraft:pattern_item/globe\001\r\035minecraft:pattern_item/guster\001\020\035minecraft:pattern_item/mojang\001\025\034minecraft:pattern_item/skull\001\030\032minecraft:painting_variant\001\023minecraft:placeable.\027\001\000\002\005\037.\"\f$)\r-\025\031\b',&1\022 \036\a%\016\004\026\032#+\003\006\t\n\v\020\021\023\030\033\034\035!(*\024minecraft:game_event\005\024minecraft:vibrations7\001\002\003\005\006\a\b\000\004\t\n\v\f\r\016\017\020\021\022\023\024\025\026\030\031\032\033\034 !\"#$&()*+,-./0123456789:;\027$minecraft:ignore_vibrations_sneaking\006\032$)*\035\034\033minecraft:warden_can_listen8\001\002\003\005\006\a\b\000\004\t\n\v\f\r\016\017\020\021\022\023\024\025\026\030\031\032\033\034 !\"#$&()*+,-./0123456789:;'%\032minecraft:allay_can_listen\001!\035minecraft:shrieker_can_listen\001% minecraft:point_of_interest_type\003\022minecraft:bee_home\002\017\020\035minecraft:acquirable_job_site\r\000\001\002\003\004\005\006\a\b\t\n\v\f\021minecraft:village\017\000\001\002\003\004\005\006\a\b\t\n\v\f\r\016\016minecraft:item\223\001\020minecraft:skulls\a\321\b\323\b\322\b\317\b\320\b\324\b\325\b\037minecraft:soul_fire_base_blocks\002\306\002\307\002\030minecraft:trim_materials\n\253\006\255\006\257\006\247\006\246\006\245\006\260\006\221\005\250\006\251\006\024minecraft:head_armor\a\330\006\334\006\350\006\340\006\344\006\354\006\232\006\036minecraft:beacon_payment_items\005\260\006\246\006\245\006\257\006\253\006\026minecraft:wooden_slabs\v\374\001\375\001\376\001\377\001\200\002\202\002\206\002\207\002\203\002\204\002\201\002\023minecraft:coal_ores\002>?\026minecraft:chicken_food\006\325\006\333\a\332\a\203\t\200\t\201\t\027minecraft:small_flowers\016\332\001\333\001\334\001\335\001\336\001\337\001\340\001\341\001\342\001\343\001\344\001\345\001\346\001\347\001\037minecraft:parrot_poisonous_food\001\324\a\032minecraft:wooden_trapdoors\v\337\005\335\005\341\005\336\005\333\005\334\005\344\005\345\005\342\005\343\005\340\005\022minecraft:pig_food\003\311\b\312\b\202\t\031minecraft:trimmable_armor\031\333\006\337\006\353\006\343\006\347\006\357\006\332\006\336\006\352\006\342\006\346\006\356\006\331\006\335\006\351\006\341\006\345\006\355\006\330\006\334\006\350\006\340\006\344\006\354\006\232\006\032minecraft:enchantable/mace\001\305\b\016minecraft:wool\020\312\001\313\001\314\001\315\001\316\001\317\001\320\001\321\001\322\001\323\001\324\001\325\001\326\001\327\001\330\001\331\001\020minecraft:stairs8\377\002\200\003\201\003\202\003\203\003\205\003\211\003\212\003\206\003\207\003\204\003\210\003\260\002\374\002\362\002\352\002\351\002\251\002\252\003\201\004\373\003\372\003\374\003\355\004\356\004\357\004\360\004\361\004\362\004\363\004\364\004\365\004\366\004\367\004\370\004\371\004\372\004\316\t\326\t\322\t\373\004\374\004\376\004\375\004kjih~}|\177\353\002\016\023\027\016minecraft:logs(\212\001\254\001\227\001\241\001\204\001\246\001\221\001\233\001\210\001\252\001\225\001\237\001\206\001\250\001\223\001\235\001\207\001\251\001\224\001\236\001\205\001\247\001\222\001\234\001\213\001\255\001\230\001\242\001\211\001\253\001\226\001\240\001\216\001\231\001\256\001\243\001\217\001\232\001\257\001\244\001\"minecraft:creeper_drop_music_discs\f\220\t\221\t\222\t\223\t\226\t\227\t\230\t\231\t\232\t\233\t\234\t\235\t\024minecraft:camel_food\001\264\002\020minecraft:arrows\003\242\006\210\t\207\t\026minecraft:wool_carpets\020\276\003\277\003\300\003\301\003\302\003\303\003\304\003\305\003\306\003\307\003\310\003\311\003\312\003\313\003\314\003\315\003\023minecraft:compasses\002\240\a\241\a\022minecraft:cow_food\001\326\006\031minecraft:bookshelf_books\005\235\a\304\b\332\b\303\b\216\t\035minecraft:enchantable/fishing\001\243\a#minecraft:decorated_pot_ingredients\030\231\a\210\n\211\n\212\n\213\n\214\n\215\n\216\n\217\n\221\n\223\n\224\n\225\n\226\n\227\n\230\n\231\n\233\n\234\n\235\n\236\n\220\n\222\n\232\n\026minecraft:wooden_doors\v\307\005\310\005\311\005\312\005\313\005\315\005\320\005\321\005\316\005\317\005\314\005\033minecraft:enchantable/sword\006\306\006\267\006\274\006\313\006\262\006\301\006\024minecraft:horse_food\a\326\006\302\a\275\003\240\006\316\b\364\006\365\006\016minecraft:meat\v\334\a\336\a\335\a\337\a\354\b\362\006\337\b\353\b\361\006\336\b\340\a\026minecraft:warped_stems\004\217\001\232\001\257\001\244\001\026minecraft:emerald_ores\002HI!minecraft:enchantable/fire_aspect\a\306\006\267\006\274\006\313\006\262\006\301\006\305\b\027minecraft:bamboo_blocks\002\220\001\245\001\027minecraft:crimson_stems\004\216\001\231\001\256\001\243\001\023minecraft:wolf_food\v\334\a\336\a\335\a\337\a\354\b\362\006\337\b\353\b\361\006\336\b\340\a\033minecraft:horse_tempt_items\003\316\b\364\006\365\006\"minecraft:ignored_by_piglin_babies\001\221\a\031minecraft:enchantable/bow\001\241\006\020minecraft:swords\006\306\006\267\006\274\006\313\006\262\006\301\006\036minecraft:stone_tool_materials\003#\314\t\t\037minecraft:enchantable/leg_armor\006\332\006\336\006\352\006\342\006\346\006\356\006\024minecraft:terracotta\021\316\003\253\003\254\003\255\003\256\003\257\003\260\003\261\003\262\003\263\003\264\003\265\003\266\003\267\003\270\003\271\003\272\003\025minecraft:wart_blocks\002\205\004\206\004\027minecraft:dark_oak_logs\004\212\001\254\001\227\001\241\001 minecraft:enchantable/durabilityC\333\006\337\006\353\006\343\006\347\006\357\006\332\006\336\006\352\006\342\006\346\006\356\006\331\006\335\006\351\006\341\006\345\006\355\006\330\006\334\006\350\006\340\006\344\006\354\006\232\006\205\006\212\t\306\006\267\006\274\006\313\006\262\006\301\006\311\006\272\006\277\006\316\006\265\006\304\006\310\006\271\006\276\006\315\006\264\006\303\006\307\006\270\006\275\006\314\006\263\006\302\006\312\006\273\006\300\006\317\006\266\006\305\006\241\006\250\t\244\t\236\006\327\a\364\t\243\a\203\006\204\006\305\b\026minecraft:strider_food\001\355\001\034minecraft:non_flammable_wood\036\217\001\232\001\257\001\244\001\216\001\231\001\256\001\243\001-.\206\002\207\002\304\005\305\005\300\002\301\002\344\005\345\005\367\005\370\005\211\003\212\003\265\005\266\005\320\005\321\005\377\006\200\a\213\a\212\a\017minecraft:coals\002\243\006\244\006\025minecraft:hoglin_food\001\354\001\025minecraft:piglin_food\002\361\006\362\006\037minecraft:breaks_decorated_pots \306\006\267\006\274\006\313\006\262\006\301\006\311\006\272\006\277\006\316\006\265\006\304\006\310\006\271\006\276\006\315\006\264\006\303\006\307\006\270\006\275\006\314\006\263\006\302\006\312\006\273\006\300\006\317\006\266\006\305\006\244\t\305\b\017minecraft:anvil\003\243\003\244\003\245\003\024minecraft:birch_logs\004\206\001\250\001\223\001\235\001\024minecraft:lapis_ores\002JK\034minecraft:enchantable/mining\031\311\006\272\006\277\006\316\006\265\006\304\006\310\006\271\006\276\006\315\006\264\006\303\006\307\006\270\006\275\006\314\006\263\006\302\006\312\006\273\006\300\006\317\006\266\006\305\006\327\a\016minecraft:axes\006\311\006\272\006\277\006\316\006\265\006\304\006\016minecraft:hoes\006\312\006\273\006\300\006\317\006\266\006\305\006\024minecraft:llama_food\002\326\006\275\003\026minecraft:sniffer_food\001\200\t minecraft:enchantable/head_armor\a\330\006\334\006\350\006\340\006\344\006\354\006\232\006\020minecraft:fences\f\267\002\273\002\275\002\270\002\271\002\272\002\300\002\301\002\276\002\277\002\274\002\361\002\022minecraft:saplings\n012346\305\001\306\00175\025minecraft:parrot_food\006\325\006\333\a\332\a\203\t\200\t\201\t\016minecraft:beds\020\322\a\323\a\317\a\320\a\315\a\313\a\321\a\307\a\314\a\311\a\306\a\305\a\312\a\316\a\304\a\310\a\023minecraft:iron_ores\002@A\025minecraft:rabbit_food\003\311\b\316\b\332\001\033minecraft:enchantable/armor\031\333\006\337\006\353\006\343\006\347\006\357\006\332\006\336\006\352\006\342\006\346\006\356\006\331\006\335\006\351\006\341\006\345\006\355\006\330\006\334\006\350\006\340\006\344\006\354\006\232\006\022minecraft:oak_logs\004\204\001\246\001\221\001\233\001\017minecraft:doors\024\307\005\310\005\311\005\312\005\313\005\315\005\320\005\321\005\316\005\317\005\314\005\322\005\323\005\324\005\325\005\326\005\327\005\330\005\331\005\306\005\025minecraft:ocelot_food\002\247\a\250\a\021minecraft:banners\020\355\b\356\b\357\b\360\b\361\b\362\b\363\b\364\b\365\b\366\b\367\b\370\b\371\b\372\b\373\b\374\b#minecraft:noteblock_top_instruments\a\322\b\317\b\323\b\324\b\320\b\325\b\321\b\031minecraft:smelts_to_glass\0029<\"minecraft:stone_crafting_materials\003#\314\t\t\033minecraft:piglin_repellents\003\313\002\277\t\303\t\027minecraft:wooden_fences\v\267\002\273\002\275\002\270\002\271\002\272\002\300\002\301\002\276\002\277\002\274\002\030minecraft:trim_templates\022\372\t\200\n\370\t\373\t\367\t\371\t\377\t\375\t\366\t\374\t\376\t\201\n\202\n\203\n\204\n\205\n\206\n\207\n\026minecraft:axolotl_food\001\226\a\"minecraft:villager_plantable_seeds\006\325\006\312\b\311\b\203\t\200\t\201\t\023minecraft:leg_armor\006\332\006\336\006\352\006\342\006\346\006\356\006 minecraft:enchantable/foot_armor\006\333\006\337\006\353\006\343\006\347\006\357\006\024minecraft:panda_food\001\373\001\034minecraft:dampens_vibrations \312\001\313\001\314\001\315\001\316\001\317\001\320\001\321\001\322\001\323\001\324\001\325\001\326\001\327\001\330\001\331\001\276\003\277\003\300\003\301\003\302\003\303\003\304\003\305\003\306\003\307\003\310\003\311\003\312\003\313\003\314\003\315\003\027minecraft:mangrove_logs\004\213\001\255\001\230\001\242\001\025minecraft:jungle_logs\004\207\001\251\001\224\001\236\001\027minecraft:lectern_books\002\304\b\303\b!minecraft:enchantable/chest_armor\006\331\006\335\006\351\006\341\006\345\006\355\006\033minecraft:llama_tempt_items\001\275\003\025minecraft:turtle_food\001\310\001\017minecraft:signs\v\366\006\367\006\370\006\372\006\371\006\374\006\377\006\200\a\375\006\376\006\373\006\027minecraft:wooden_stairs\v\377\002\200\003\201\003\202\003\203\003\205\003\211\003\212\003\206\003\207\003\204\003\025minecraft:spruce_logs\004\205\001\247\001\222\001\234\001\034minecraft:enchantable/weapon\r\306\006\267\006\274\006\313\006\262\006\301\006\311\006\272\006\277\006\316\006\265\006\304\006\305\b\030minecraft:wooden_buttons\v\254\005\255\005\256\005\257\005\260\005\262\005\265\005\266\005\263\005\264\005\261\005\020minecraft:fishes\006\247\a\253\a\250\a\254\a\252\a\251\a\026minecraft:stone_bricks\004\324\002\325\002\326\002\327\002\021minecraft:shovels\006\307\006\270\006\275\006\314\006\263\006\302\006\025minecraft:chest_boats\t\207\006\211\006\213\006\215\006\217\006\223\006\225\006\227\006\221\006\032minecraft:creeper_igniters\002\236\006\301\b minecraft:enchantable/equippable\"\333\006\337\006\353\006\343\006\347\006\357\006\332\006\336\006\352\006\342\006\346\006\356\006\331\006\335\006\351\006\341\006\345\006\355\006\330\006\334\006\350\006\340\006\344\006\354\006\232\006\205\006\321\b\323\b\322\b\317\b\320\b\324\b\325\b\303\002\035minecraft:enchantable/trident\001\244\t\017minecraft:slabs<\374\001\375\001\376\001\377\001\200\002\202\002\206\002\207\002\203\002\204\002\201\002\205\002\210\002\211\002\217\002\212\002\225\002\222\002\223\002\216\002\215\002\221\002\214\002\226\002\227\002\230\002\377\004\200\005\201\005\202\005\203\005\204\005\205\005\206\005\207\005\210\005\211\005\212\005\213\005\213\002\224\002\315\t\325\t\321\t\214\005\215\005\217\005\216\005\202\001\201\001\200\001onml\203\001\220\002\r\022\026\"minecraft:enchantable/sharp_weapon\f\306\006\267\006\274\006\313\006\262\006\301\006\311\006\272\006\277\006\316\006\265\006\304\006!minecraft:enchantable/mining_loot\030\311\006\272\006\277\006\316\006\265\006\304\006\310\006\271\006\276\006\315\006\264\006\303\006\307\006\270\006\275\006\314\006\263\006\302\006\312\006\273\006\300\006\317\006\266\006\305\006\037minecraft:enchantable/vanishingL\333\006\337\006\353\006\343\006\347\006\357\006\332\006\336\006\352\006\342\006\346\006\356\006\331\006\335\006\351\006\341\006\345\006\355\006\330\006\334\006\350\006\340\006\344\006\354\006\232\006\205\006\212\t\306\006\267\006\274\006\313\006\262\006\301\006\311\006\272\006\277\006\316\006\265\006\304\006\310\006\271\006\276\006\315\006\264\006\303\006\307\006\270\006\275\006\314\006\263\006\302\006\312\006\273\006\300\006\317\006\266\006\305\006\241\006\250\t\244\t\236\006\327\a\364\t\243\a\203\006\204\006\305\b\240\a\303\002\321\b\323\b\322\b\317\b\320\b\324\b\325\b\027minecraft:hanging_signs\v\201\a\202\a\203\a\205\a\206\a\204\a\207\a\212\a\213\a\210\a\211\a\023minecraft:trapdoors\024\337\005\335\005\341\005\336\005\333\005\334\005\344\005\345\005\342\005\343\005\340\005\332\005\346\005\347\005\350\005\351\005\352\005\353\005\354\005\355\005\027minecraft:redstone_ores\002FG\025minecraft:cherry_logs\004\211\001\253\001\226\001\240\001\021minecraft:buttons\r\254\005\255\005\256\005\257\005\260\005\262\005\265\005\266\005\263\005\264\005\261\005\252\005\253\005\021minecraft:flowers\032\332\001\333\001\334\001\335\001\336\001\337\001\340\001\341\001\342\001\343\001\344\001\345\001\346\001\347\001\321\003\322\003\324\003\323\003\350\001\271\001\306\0017\265\001\366\001\246\002\351\001\021minecraft:dyeable\006\330\006\331\006\332\006\333\006\347\b\235\006\020minecraft:planks\v$%&'(*-.+,)\022minecraft:fox_food\002\300\t\301\t\027minecraft:stone_buttons\002\252\005\253\005\017minecraft:boats\022\206\006\210\006\212\006\214\006\216\006\222\006\224\006\226\006\220\006\207\006\211\006\213\006\215\006\217\006\223\006\225\006\227\006\221\006\036minecraft:enchantable/crossbow\001\250\t\025minecraft:chest_armor\006\331\006\335\006\351\006\341\006\345\006\355\006\023minecraft:frog_food\001\236\a\017minecraft:rails\004\373\005\371\005\372\005\374\005\026minecraft:diamond_ores\002LM\020minecraft:leaves\n\263\001\260\001\261\001\266\001\264\001\262\001\270\001\271\001\267\001\265\001\035minecraft:strider_tempt_items\002\355\001\204\006\017minecraft:walls\031\215\003\216\003\217\003\220\003\221\003\222\003\223\003\224\003\226\003\227\003\230\003\231\003\232\003\233\003\234\003\236\003\235\003\237\003\240\003\242\003\241\003\225\003\017\024\030\025minecraft:fence_gates\v\362\005\360\005\364\005\361\005\356\005\357\005\367\005\370\005\365\005\366\005\363\005\030minecraft:armadillo_food\001\350\a\023minecraft:goat_food\001\326\006 minecraft:wooden_pressure_plates\v\273\005\274\005\275\005\276\005\277\005\301\005\304\005\305\005\302\005\303\005\300\005\025minecraft:acacia_logs\004\210\001\252\001\225\001\237\001\024minecraft:sheep_food\001\326\006\022minecraft:cat_food\002\247\a\250\a\026minecraft:piglin_loved\031DNEZ\317\t\271\005\257\006\275\t\244\a\316\b\357\a\364\006\365\006\350\006\351\006\352\006\353\006\345\b\274\006\276\006\275\006\277\006\300\006\256\006T\021minecraft:candles\021\331\t\332\t\333\t\334\t\335\t\336\t\337\t\340\t\341\t\342\t\343\t\344\t\345\t\346\t\347\t\350\t\351\t\026minecraft:tall_flowers\005\321\003\322\003\324\003\323\003\350\001\022minecraft:bee_food\032\332\001\333\001\334\001\335\001\336\001\337\001\340\001\341\001\342\001\343\001\344\001\345\001\346\001\347\001\321\003\322\003\324\003\323\003\350\001\271\001\306\0017\265\001\366\001\246\002\351\001\025minecraft:copper_ores\002BC\016minecraft:sand\0039<:\023minecraft:gold_ores\003DNE!minecraft:freeze_immune_wearables\005\333\006\332\006\331\006\330\006\347\b&minecraft:completes_find_tree_tutorial4\212\001\254\001\227\001\241\001\204\001\246\001\221\001\233\001\210\001\252\001\225\001\237\001\206\001\250\001\223\001\235\001\207\001\251\001\224\001\236\001\205\001\247\001\222\001\234\001\213\001\255\001\230\001\242\001\211\001\253\001\226\001\240\001\216\001\231\001\256\001\243\001\217\001\232\001\257\001\244\001\263\001\260\001\261\001\266\001\264\001\262\001\270\001\271\001\267\001\265\001\205\004\206\004\030minecraft:logs_that_burn \212\001\254\001\227\001\241\001\204\001\246\001\221\001\233\001\210\001\252\001\225\001\237\001\206\001\250\001\223\001\235\001\207\001\251\001\224\001\236\001\205\001\247\001\222\001\234\001\213\001\255\001\230\001\242\001\211\001\253\001\226\001\240\001\016minecraft:dirt\t\034\033\036\035\354\002\037\367\001 \215\001\036minecraft:decorated_pot_sherds\027\210\n\211\n\212\n\213\n\214\n\215\n\216\n\217\n\221\n\223\n\224\n\225\n\226\n\227\n\230\n\231\n\233\n\234\n\235\n\236\n\220\n\222\n\232\n\022minecraft:pickaxes\006\310\006\271\006\276\006\315\006\264\006\303\006\024minecraft:foot_armor\006\333\006\337\006\353\006\343\006\347\006\357\006\"minecraft:cluster_max_harvestables\006\310\006\276\006\303\006\315\006\271\006\264\006"
#define ACTUAL_SIMULATION_DISTANCE RENDER_DISTANCE <= SIMULATION_DISTANCE ? RENDER_DISTANCE : SIMULATION_DISTANCE

// config
#define ADDRESS "0.0.0.0"
#define FAVICON
#define MAX_PLAYERS 15
#define PORT 25565
#define RENDER_DISTANCE 2
#define SIMULATION_DISTANCE 5

struct ThreadArguments
{
	int client_endpoint;
	uint8_t *interthread_buffer;
	size_t *interthread_buffer_length;
	pthread_mutex_t *interthread_buffer_mutex;
	pthread_cond_t *interthread_buffer_condition;
	sem_t *client_thread_arguments_semaphore;
	enum State *connection_state;
};

static void *
packet_receiver(void *thread_arguments)
{
	{
		const int client_endpoint = ((struct ThreadArguments*)thread_arguments)->client_endpoint;
		uint8_t * const interthread_buffer = ((struct ThreadArguments*)thread_arguments)->interthread_buffer;
		size_t * const interthread_buffer_length = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_length;
		pthread_mutex_t * const interthread_buffer_mutex = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_mutex;
		pthread_cond_t * const interthread_buffer_condition = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_condition;
		enum State * const connection_state = ((struct ThreadArguments*)thread_arguments)->connection_state;
		if (unlikely(sem_post(((struct ThreadArguments*)thread_arguments)->client_thread_arguments_semaphore) == -1))
			goto clear_stack_receiver;
		Boolean compression_enabled = false;
		int8_t buffer[PACKET_MAXSIZE];
		uint8_t packet_next_boundary;
		size_t buffer_offset;
		int ret;
		while (1)
		{
			const ssize_t bytes_read = recv(client_endpoint, buffer, PACKET_MAXSIZE, 0);
			if (!bytes_read) return NULL;
			else if (unlikely(bytes_read == -1)) goto clear_stack_receiver;
			bullshitcore_network_varint_decode(buffer, &packet_next_boundary);
			buffer_offset = packet_next_boundary;
			const uint32_t packet_identifier = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
			buffer_offset += packet_next_boundary;
			switch (*connection_state)
			{
				case State_Handshaking:
				{
					switch (packet_identifier)
					{
						case Packet_Handshake:
						{
							const uint32_t client_protocol_version = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							if (client_protocol_version != PROTOCOL_VERSION)
								bullshitcore_log_log("Warning! Client and server protocol version mismatch.");
							const uint32_t server_address_string_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							const uint32_t target_state = bullshitcore_network_varint_decode(buffer + buffer_offset + server_address_string_length + 2, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							if (target_state >= State_Status && target_state <= State_Transfer)
								*connection_state = target_state;
							break;
						}
					}
					break;
				}
				case State_Status:
				{
					switch (packet_identifier)
					{
						case Packet_Status_Client_Status_Request:
						{
							// TODO: Sanitise input (implement it after testing)
							const uint8_t * const text = (const uint8_t *)"{\"version\":{\"name\":\"" MINECRAFT_VERSION "\",\"protocol\":" EXPAND_AND_STRINGIFY(PROTOCOL_VERSION) "},\"players\":{\"max\":" EXPAND_AND_STRINGIFY(MAX_PLAYERS) ",\"online\":0},\"description\":{\"text\":\"BullshitCore is up and running!\"},\"favicon\":\"" FAVICON "\"}";
							const size_t text_length = strlen((const char *)text);
							const JSONTextComponent packet_payload =
							{
								bullshitcore_network_varint_encode(text_length),
								text
							};
							if (text_length > JSONTEXTCOMPONENT_MAXSIZE) break;
							uint8_t packet_payload_length_length;
							bullshitcore_network_varint_decode(packet_payload.length, &packet_payload_length_length);
							const size_t packet_length = 1 + packet_payload_length_length + text_length;
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_length);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Status_Server_Status_Response);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)packet_payload.length, packet_payload_length_length,
								(uintptr_t)packet_payload.contents, text_length)
							free(packet_payload.length);
							free(packet_length_varint);
							free(packet_identifier_varint);
							break;
						}
						case Packet_Status_Client_Ping_Request:
						{
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Status_Server_Ping_Response);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length + 8);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)buffer, 8)
							free(packet_identifier_varint);
							free(packet_length_varint);
							goto close_connection;
							break;
						}
					}
					break;
				}
				case State_Login:
				{
					switch (packet_identifier)
					{
						case Packet_Login_Client_Login_Start:
						{
							uint8_t username_length_length;
							const uint32_t username_length = bullshitcore_network_varint_decode(buffer + buffer_offset, &username_length_length);
							buffer_offset += username_length_length + username_length;
							const uint32_t packet_length = username_length_length + username_length + 19;
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_length);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Login_Server_Login_Success);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const properties_count_varint = bullshitcore_network_varint_encode(0);
							uint8_t properties_count_varint_length;
							bullshitcore_network_varint_decode(properties_count_varint, &properties_count_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)(buffer + buffer_offset), 16,
								(uintptr_t)(buffer + buffer_offset - username_length_length - username_length), username_length_length + username_length,
								(uintptr_t)properties_count_varint, properties_count_varint_length,
								(uintptr_t)&(Boolean){ true }, sizeof(Boolean))
							free(packet_length_varint);
							free(packet_identifier_varint);
							free(properties_count_varint);
							break;
						}
						case Packet_Login_Client_Encryption_Response:
						{
							break;
						}
						case Packet_Login_Client_Login_Plugin_Response:
						{
							break;
						}
						case Packet_Login_Client_Login_Acknowledged:
						{
							*connection_state = State_Configuration;
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Configuration_Server_Known_Packs);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const known_packs_count_varint = bullshitcore_network_varint_encode(1);
							uint8_t known_packs_count_varint_length;
							bullshitcore_network_varint_decode(known_packs_count_varint, &known_packs_count_varint_length);
							const String namespace = { bullshitcore_network_varint_encode(strlen("minecraft")), (const uint8_t *)"minecraft" };
							const String identifier = { bullshitcore_network_varint_encode(strlen("core")), (const uint8_t *)"core" };
							const String version = { bullshitcore_network_varint_encode(strlen("1.21")), (const uint8_t *)"1.21" };
							uint8_t namespace_length_varint_length;
							bullshitcore_network_varint_decode(namespace.length, &namespace_length_varint_length);
							uint8_t identifier_length_varint_length;
							bullshitcore_network_varint_decode(identifier.length, &identifier_length_varint_length);
							uint8_t version_length_varint_length;
							bullshitcore_network_varint_decode(version.length, &version_length_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
								+ known_packs_count_varint_length
								+ namespace_length_varint_length
								+ strlen((const char *)namespace.contents)
								+ identifier_length_varint_length
								+ strlen((const char *)identifier.contents)
								+ version_length_varint_length
								+ strlen((const char *)version.contents));
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)known_packs_count_varint, known_packs_count_varint_length,
								(uintptr_t)namespace.length, namespace_length_varint_length,
								(uintptr_t)namespace.contents, strlen((const char *)namespace.contents),
								(uintptr_t)identifier.length, identifier_length_varint_length,
								(uintptr_t)identifier.contents, strlen((const char *)identifier.contents),
								(uintptr_t)version.length, version_length_varint_length,
								(uintptr_t)version.contents, strlen((const char *)version.contents))
							free(packet_identifier_varint);
							free(known_packs_count_varint);
							free(namespace.length);
							free(identifier.length);
							free(version.length);
							free(packet_length_varint);
							break;
						}
						case Packet_Login_Client_Cookie_Response:
						{
							break;
						}
					}
					break;
				}
				case State_Transfer:
				{
					break;
				}
				case State_Configuration:
				{
					switch (packet_identifier)
					{
						case Packet_Configuration_Client_Client_Information:
						{
							break;
						}
						case Packet_Configuration_Client_Cookie_Response:
						{
							break;
						}
						case Packet_Configuration_Client_Plugin_Message:
						{
							break;
						}
						case Packet_Configuration_Client_Finish_Configuration_Acknowledge:
						{
							*connection_state = State_Play;
							VarInt *packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Play_Server_Login);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							const uint8_t * const overworld = (const uint8_t *)"minecraft:overworld";
							const uint8_t * const nether = (const uint8_t *)"minecraft:the_nether";
							const uint8_t * const end = (const uint8_t *)"minecraft:the_end";
							const Identifier dimensions[] =
							{
								{
									bullshitcore_network_varint_encode(strlen((const char *)overworld)),
									overworld
								},
								{
									bullshitcore_network_varint_encode(strlen((const char *)nether)),
									nether
								},
								{
									bullshitcore_network_varint_encode(strlen((const char *)end)),
									end
								}
							};
							VarInt * const dimension_count_varint = bullshitcore_network_varint_encode(NUMOF(dimensions));
							uint8_t dimension_count_varint_length;
							bullshitcore_network_varint_decode(dimension_count_varint, &dimension_count_varint_length);
							uint8_t dimension_1_length_length;
							bullshitcore_network_varint_decode(dimensions[0].length, &dimension_1_length_length);
							uint8_t dimension_2_length_length;
							bullshitcore_network_varint_decode(dimensions[1].length, &dimension_2_length_length);
							uint8_t dimension_3_length_length;
							bullshitcore_network_varint_decode(dimensions[2].length, &dimension_3_length_length);
							VarInt * const max_players_varint = bullshitcore_network_varint_encode(MAX_PLAYERS);
							uint8_t max_players_varint_length;
							bullshitcore_network_varint_decode(max_players_varint, &max_players_varint_length);
							VarInt * const render_distance_varint = bullshitcore_network_varint_encode(RENDER_DISTANCE);
							uint8_t render_distance_varint_length;
							bullshitcore_network_varint_decode(render_distance_varint, &render_distance_varint_length);
							VarInt * const simulation_distance_varint = bullshitcore_network_varint_encode(ACTUAL_SIMULATION_DISTANCE);
							uint8_t simulation_distance_varint_length;
							bullshitcore_network_varint_decode(simulation_distance_varint, &simulation_distance_varint_length);
							VarInt * const dimension_type_varint = bullshitcore_network_varint_encode(0);
							uint8_t dimension_type_varint_length;
							bullshitcore_network_varint_decode(dimension_type_varint, &dimension_type_varint_length);
							int64_t seed_hash = -1916599016670012116; // leaked government information??
							ret = wc_Sha256Hash((const byte *)&seed_hash, sizeof seed_hash, (byte *)&seed_hash);
							if (unlikely(ret))
							{
								fprintf(stderr, "wc_Sha256Hash: %s\n", wc_GetErrorString(ret));
								goto clear_stack_receiver;
							}
							VarInt * const portal_cooldown_varint = bullshitcore_network_varint_encode(0);
							uint8_t portal_cooldown_varint_length;
							bullshitcore_network_varint_decode(portal_cooldown_varint, &portal_cooldown_varint_length);
							VarInt *packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
								+ sizeof(int32_t)
								+ sizeof(Boolean)
								+ dimension_count_varint_length
								+ dimension_1_length_length
								+ bullshitcore_network_varint_decode(dimensions[0].length, NULL)
								+ dimension_2_length_length
								+ bullshitcore_network_varint_decode(dimensions[1].length, NULL)
								+ dimension_3_length_length
								+ bullshitcore_network_varint_decode(dimensions[2].length, NULL)
								+ max_players_varint_length
								+ render_distance_varint_length
								+ simulation_distance_varint_length
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ dimension_type_varint_length
								+ dimension_1_length_length
								+ bullshitcore_network_varint_decode(dimensions[0].length, NULL)
								+ sizeof seed_hash
								+ sizeof(uint8_t)
								+ sizeof(int8_t)
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ sizeof(Boolean)
								+ portal_cooldown_varint_length
								+ sizeof(Boolean));
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							VarInt * const packet_2_identifier_varint = bullshitcore_network_varint_encode(Packet_Play_Server_Game_Event);
							uint8_t packet_2_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_2_identifier_varint, &packet_2_identifier_varint_length);
							VarInt * const packet_2_length_varint = bullshitcore_network_varint_encode(packet_2_identifier_varint_length
								+ sizeof(uint8_t)
								+ (sizeof(float) >= 4 ? 4 : sizeof(float)));
							uint8_t packet_2_length_varint_length;
							bullshitcore_network_varint_decode(packet_2_length_varint, &packet_2_length_varint_length);
							SEND((uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length,
								(uintptr_t)&(const int32_t){ 0 }, sizeof(int32_t),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)dimension_count_varint, dimension_count_varint_length,
								(uintptr_t)dimensions[0].length, dimension_1_length_length,
								(uintptr_t)dimensions[0].contents, bullshitcore_network_varint_decode(dimensions[0].length, NULL),
								(uintptr_t)dimensions[1].length, dimension_2_length_length,
								(uintptr_t)dimensions[1].contents, bullshitcore_network_varint_decode(dimensions[1].length, NULL),
								(uintptr_t)dimensions[2].length, dimension_3_length_length,
								(uintptr_t)dimensions[2].contents, bullshitcore_network_varint_decode(dimensions[2].length, NULL),
								(uintptr_t)max_players_varint, max_players_varint_length,
								(uintptr_t)render_distance_varint, render_distance_varint_length,
								(uintptr_t)simulation_distance_varint, simulation_distance_varint_length,
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ true }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)dimension_type_varint, dimension_type_varint_length,
								(uintptr_t)dimensions[0].length, dimension_1_length_length,
								(uintptr_t)dimensions[0].contents, bullshitcore_network_varint_decode(dimensions[0].length, NULL),
								(uintptr_t)&seed_hash, sizeof seed_hash,
								(uintptr_t)&(const uint8_t){ 3 }, sizeof(uint8_t),
								(uintptr_t)&(const int8_t){ -1 }, sizeof(int8_t),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)portal_cooldown_varint, portal_cooldown_varint_length,
								(uintptr_t)&(const Boolean){ false }, sizeof(Boolean),
								(uintptr_t)packet_2_length_varint, packet_2_length_varint_length,
								(uintptr_t)packet_2_identifier_varint, packet_2_identifier_varint_length,
								(uintptr_t)&(uint8_t){ 13 }, sizeof(uint8_t),
								(uintptr_t)&(float){ 0 }, sizeof(float) >= 4 ? 4 : sizeof(float))
							free(packet_identifier_varint);
							free(dimension_count_varint);
							free(dimensions[0].length);
							free(dimensions[1].length);
							free(dimensions[2].length);
							free(max_players_varint);
							free(render_distance_varint);
							free(simulation_distance_varint);
							free(dimension_type_varint);
							free(portal_cooldown_varint);
							free(packet_length_varint);
							free(packet_2_identifier_varint);
							free(packet_2_length_varint);
							break;
						}
						case Packet_Configuration_Client_Keep_Alive:
						{
							break;
						}
						case Packet_Configuration_Client_Pong:
						{
							break;
						}
						case Packet_Configuration_Client_Resource_Pack_Response:
						{
							break;
						}
						case Packet_Configuration_Client_Known_Packs:
						{
							const uint32_t client_known_packs = bullshitcore_network_varint_decode(buffer + buffer_offset, &packet_next_boundary);
							buffer_offset += packet_next_boundary;
							VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(Packet_Configuration_Server_Finish_Configuration);
							uint8_t packet_identifier_varint_length;
							bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
							VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length);
							uint8_t packet_length_varint_length;
							bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
							SEND((uintptr_t)REGISTRY_1, sizeof REGISTRY_1,
								(uintptr_t)REGISTRY_2, sizeof REGISTRY_2,
								(uintptr_t)REGISTRY_3, sizeof REGISTRY_3,
								(uintptr_t)REGISTRY_4, sizeof REGISTRY_4,
								(uintptr_t)REGISTRY_5, sizeof REGISTRY_5,
								(uintptr_t)REGISTRY_6, sizeof REGISTRY_6,
								(uintptr_t)REGISTRY_7, sizeof REGISTRY_7,
								(uintptr_t)REGISTRY_8, sizeof REGISTRY_8,
								(uintptr_t)REGISTRY_9, sizeof REGISTRY_9,
								(uintptr_t)REGISTRY_10, sizeof REGISTRY_10,
								(uintptr_t)REGISTRY_11, sizeof REGISTRY_11,
								(uintptr_t)REGISTRY_12, sizeof REGISTRY_12,
								(uintptr_t)packet_length_varint, packet_length_varint_length,
								(uintptr_t)packet_identifier_varint, packet_identifier_varint_length)
							free(packet_identifier_varint);
							free(packet_length_varint);
							break;
						}
					}
					break;
				}
				case State_Play:
				{
					break;
				}
			}
#ifndef NDEBUG
			bullshitcore_log_log("Reached an end of the packet jump table.");
#endif
		}
	}
close_connection:
	return NULL;
clear_stack_receiver:;
	// TODO: Also pass failed function name
	const int my_errno = errno;
#ifndef NDEBUG
	bullshitcore_log_logf("Receiver thread crashed! %s\n", strerror(my_errno));
#endif
	int * const p_my_errno = malloc(sizeof my_errno);
	if (unlikely(!p_my_errno)) return (void *)1;
	*p_my_errno = my_errno;
	return p_my_errno;
}

static void *
packet_sender(void *thread_arguments)
{
	{
		const int client_endpoint = ((struct ThreadArguments*)thread_arguments)->client_endpoint;
		uint8_t * const interthread_buffer = ((struct ThreadArguments*)thread_arguments)->interthread_buffer;
		size_t * const interthread_buffer_length = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_length;
		pthread_mutex_t * const interthread_buffer_mutex = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_mutex;
		pthread_cond_t * const interthread_buffer_condition = ((struct ThreadArguments*)thread_arguments)->interthread_buffer_condition;
		enum State * const connection_state = ((struct ThreadArguments*)thread_arguments)->connection_state;
		if (unlikely(sem_post(((struct ThreadArguments*)thread_arguments)->client_thread_arguments_semaphore) == -1))
			goto clear_stack_sender;
		struct timespec keep_alive_timeout;
		while (1)
		{
			int ret = pthread_mutex_lock(interthread_buffer_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
			if (unlikely(clock_gettime(CLOCK_REALTIME, &keep_alive_timeout) == -1))
				goto clear_stack_sender;
			keep_alive_timeout.tv_sec += 15;
			ret = pthread_cond_timedwait(interthread_buffer_condition, interthread_buffer_mutex, &keep_alive_timeout);
			if (ret == ETIMEDOUT)
			{
				int32_t packet_identifier = *connection_state == State_Configuration
					? Packet_Configuration_Server_Keep_Alive
					: Packet_Play_Server_Keep_Alive;
				VarInt * const packet_identifier_varint = bullshitcore_network_varint_encode(packet_identifier);
				uint8_t packet_identifier_varint_length;
				bullshitcore_network_varint_decode(packet_identifier_varint, &packet_identifier_varint_length);
				VarInt * const packet_length_varint = bullshitcore_network_varint_encode(packet_identifier_varint_length
					+ sizeof(int64_t));
				uint8_t packet_length_varint_length;
				bullshitcore_network_varint_decode(packet_length_varint, &packet_length_varint_length);
				memcpy(interthread_buffer, packet_length_varint, packet_length_varint_length);
				size_t interthread_buffer_offset = packet_length_varint_length;
				memcpy(interthread_buffer + interthread_buffer_offset, packet_identifier_varint, packet_identifier_varint_length);
				interthread_buffer_offset += packet_identifier_varint_length;
				memcpy(interthread_buffer + interthread_buffer_offset, &(int64_t){ 0 }, sizeof(int64_t));
				*interthread_buffer_length = interthread_buffer_offset + sizeof(int64_t);
			}
			else if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
			if (unlikely(send(client_endpoint, interthread_buffer, *interthread_buffer_length, 0) == -1))
				goto clear_stack_sender;
			ret = pthread_mutex_unlock(interthread_buffer_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				goto clear_stack_sender;
			}
		}
	}
	return NULL;
clear_stack_sender:;
	const int my_errno = errno;
#ifndef NDEBUG
	bullshitcore_log_logf("Sender thread crashed! %s\n", strerror(my_errno));
#endif
	int * const p_my_errno = malloc(sizeof my_errno);
	if (unlikely(!p_my_errno)) return (void *)1;
	*p_my_errno = my_errno;
	return p_my_errno;
}

int
main(void)
{
	pthread_attr_t thread_attributes;
	int ret = pthread_attr_init(&thread_attributes);
	if (unlikely(ret))
	{
		errno = ret;
		PERROR_AND_EXIT("pthread_attr_init")
	}
	ret = pthread_attr_setstacksize(&thread_attributes, THREAD_STACK_SIZE);
	if (unlikely(ret))
	{
		errno = ret;
		PERROR_AND_GOTO_DESTROY("pthread_attr_setstacksize", thread_attributes)
	}
	{
		const int server_endpoint = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (unlikely(server_endpoint == -1))
			PERROR_AND_GOTO_DESTROY("socket", thread_attributes)
		if (unlikely(setsockopt(server_endpoint, IPPROTO_TCP, TCP_NODELAY, &(const int){ 1 }, sizeof(int)) == -1))
			PERROR_AND_GOTO_DESTROY("setsockopt", server_endpoint)
		{
			struct in_addr address;
			ret = inet_pton(AF_INET, ADDRESS, &address);
			if (!ret)
			{
				fputs("An invalid server address was specified.\n", stderr);
				return EXIT_FAILURE;
			}
			else if (unlikely(ret == -1))
				PERROR_AND_GOTO_DESTROY("inet_pton", server_endpoint)
			const struct sockaddr_in server_address = { AF_INET, htons(PORT), address };
			struct sockaddr server_address_data;
			memcpy(&server_address_data, &server_address, sizeof server_address_data);
			if (unlikely(bind(server_endpoint, &server_address_data, sizeof server_address_data) == -1))
				PERROR_AND_GOTO_DESTROY("bind", server_endpoint)
		}
		if (unlikely(listen(server_endpoint, SOMAXCONN) == -1))
			PERROR_AND_GOTO_DESTROY("listen", server_endpoint)
		bullshitcore_log_log("Initialisation is complete, waiting for new connections.");
		{
			int client_endpoint;
			{
				while (1)
				{
					client_endpoint = accept(server_endpoint, NULL, NULL);
					if (unlikely(client_endpoint == -1))
						PERROR_AND_GOTO_DESTROY("accept", server_endpoint)
					bullshitcore_log_log("Connection is established!");
					struct ThreadArguments *thread_arguments = malloc(sizeof *thread_arguments);
					if (unlikely(!thread_arguments))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					uint8_t *interthread_buffer = malloc(sizeof *interthread_buffer * PACKET_MAXSIZE); // free me
					if (unlikely(!interthread_buffer))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					size_t *interthread_buffer_length = malloc(sizeof *interthread_buffer_length); // free me
					if (unlikely(!interthread_buffer_length))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					pthread_mutex_t *interthread_buffer_mutex = malloc(sizeof *interthread_buffer_mutex); // free me
					if (unlikely(!interthread_buffer_mutex))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					ret = pthread_mutex_init(interthread_buffer_mutex, NULL);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_mutex_init", client_endpoint)
					}
					pthread_cond_t *interthread_buffer_condition = malloc(sizeof *interthread_buffer_condition); // free me
					if (unlikely(!interthread_buffer_condition))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					ret = pthread_cond_init(interthread_buffer_condition, NULL);
					if (unlikely(ret))
					{
						errno = ret;
						PERROR_AND_GOTO_DESTROY("pthread_cond_init", client_endpoint)
					}
					sem_t *client_thread_arguments_semaphore = malloc(sizeof *client_thread_arguments_semaphore);
					if (unlikely(!client_thread_arguments_semaphore))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					if (unlikely(sem_init(client_thread_arguments_semaphore, 0, 0) == -1))
						PERROR_AND_GOTO_DESTROY("sem_init", client_endpoint)
					enum State *connection_state = malloc(sizeof *connection_state); // free me
					if (unlikely(!connection_state))
						PERROR_AND_GOTO_DESTROY("malloc", client_endpoint)
					*connection_state = State_Handshaking;
					*thread_arguments = (struct ThreadArguments)
					{
						client_endpoint,
						interthread_buffer,
						interthread_buffer_length,
						interthread_buffer_mutex,
						interthread_buffer_condition,
						client_thread_arguments_semaphore,
						connection_state
					};
					{
						pthread_t packet_receiver_thread;
						ret = pthread_create(&packet_receiver_thread, &thread_attributes, packet_receiver, thread_arguments);
						if (unlikely(ret))
						{
							errno = ret;
							PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
						}
					}
					{
						pthread_t packet_sender_thread;
						ret = pthread_create(&packet_sender_thread, &thread_attributes, packet_sender, thread_arguments);
						if (unlikely(ret))
						{
							errno = ret;
							PERROR_AND_GOTO_DESTROY("pthread_create", client_endpoint)
						}
					}
					if (unlikely(sem_wait(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
					if (unlikely(sem_wait(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_wait", client_endpoint)
					if (unlikely(sem_destroy(client_thread_arguments_semaphore) == -1))
						PERROR_AND_GOTO_DESTROY("sem_destroy", client_endpoint)
					free(client_thread_arguments_semaphore);
					free(thread_arguments);
				}
			}
			if (unlikely(close(client_endpoint) == -1))
				PERROR_AND_GOTO_DESTROY("close", server_endpoint)
			if (unlikely(close(server_endpoint) == -1)) PERROR_AND_EXIT("close")
			ret = pthread_attr_destroy(&thread_attributes);
			if (unlikely(ret))
			{
				errno = ret;
				PERROR_AND_GOTO_DESTROY("pthread_attr_destroy", client_endpoint)
			}
			return EXIT_SUCCESS;
destroy_client_endpoint:
			if (unlikely(close(client_endpoint) == -1)) perror("close");
		}
destroy_server_endpoint:
		if (unlikely(close(server_endpoint) == -1)) perror("close");
	}
destroy_thread_attributes:
	ret = pthread_attr_destroy(&thread_attributes);
	if (unlikely(ret))
	{
		errno = ret;
		perror("pthread_attr_destroy");
	}
	return EXIT_FAILURE;
}
