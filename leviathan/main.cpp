// Leviathan v17 - deep-sea dragon bot (UNSW Battlecode).
// One process per dragon. Each turn: parse vision, recognise the map (embedded copies in maps.hpp;
// any other map - 7x7 up to 256x256 - is learned as it is seen), update long-term memory (edges,
// portals, pearl countdowns, symmetry, farm pockets), then pick an action by scoring candidate moves
// with exact self-simulation, survival search, threat analysis and a beam-search pearl route
// planner. Per-map strategy for the known maps lives in params.hpp (P_MAP_TABLE).

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "params.hpp"
#include "maps.hpp"

#if !defined(LEVIATHAN_PARAMS_VERSION) || LEVIATHAN_PARAMS_VERSION != 18
#error "params.hpp does not belong to this main.cpp: use main.cpp, params.hpp and maps.hpp from the same release"
#endif
// Parameter sanity checks: if the build stops on one of these lines, that parameter in params.hpp
// has lost its value (every #define there needs a number).
namespace param_check {
[[maybe_unused]] constexpr double chk_P_MAPDB = (P_MAPDB);
[[maybe_unused]] constexpr double chk_P_UNSEEN_SURV = (P_UNSEEN_SURV);
[[maybe_unused]] constexpr double chk_P_UNSEEN_EARLY = (P_UNSEEN_EARLY);
[[maybe_unused]] constexpr double chk_P_RESPAWN_SURV = (P_RESPAWN_SURV);
[[maybe_unused]] constexpr double chk_P_DECAY = (P_DECAY);
[[maybe_unused]] constexpr double chk_P_FARM_MAXGAP = (P_FARM_MAXGAP);
[[maybe_unused]] constexpr double chk_P_FARM_ARRIVE = (P_FARM_ARRIVE);
[[maybe_unused]] constexpr double chk_P_HAZARD_UNSEEN = (P_HAZARD_UNSEEN);
[[maybe_unused]] constexpr double chk_P_GAP_PRIOR = (P_GAP_PRIOR);
[[maybe_unused]] constexpr double chk_P_UNKNOWN_PRIOR = (P_UNKNOWN_PRIOR);
[[maybe_unused]] constexpr double chk_P_CALIB = (P_CALIB);
[[maybe_unused]] constexpr double chk_P_CALIB_K = (P_CALIB_K);
[[maybe_unused]] constexpr double chk_P_CALIB_FORGET = (P_CALIB_FORGET);
[[maybe_unused]] constexpr double chk_P_CALIB_MAX = (P_CALIB_MAX);
[[maybe_unused]] constexpr double chk_P_KING_EXPO = (P_KING_EXPO);
[[maybe_unused]] constexpr double chk_P_PORTAL_BLIND_CAUTIOUS = (P_PORTAL_BLIND_CAUTIOUS);
[[maybe_unused]] constexpr double chk_P_PORTAL_EXIT_STAY_CAUTIOUS = (P_PORTAL_EXIT_STAY_CAUTIOUS);
[[maybe_unused]] constexpr double chk_P_START_EXTRAP = (P_START_EXTRAP);
[[maybe_unused]] constexpr double chk_P_ZONE_FRAC = (P_ZONE_FRAC);
[[maybe_unused]] constexpr double chk_P_ADAPT_CFG = (P_ADAPT_CFG);
[[maybe_unused]] constexpr double chk_P_BIG_MAP_TILES = (P_BIG_MAP_TILES);
[[maybe_unused]] constexpr double chk_P_SMALL_GROW_ROUND = (P_SMALL_GROW_ROUND);
[[maybe_unused]] constexpr double chk_P_SMALL_FEED_START = (P_SMALL_FEED_START);
[[maybe_unused]] constexpr double chk_P_SMALL_FEED_PULL_START = (P_SMALL_FEED_PULL_START);
[[maybe_unused]] constexpr double chk_P_FEED_KEEP_FRAC = (P_FEED_KEEP_FRAC);
[[maybe_unused]] constexpr double chk_P_FEED_KING_ONLY = (P_FEED_KING_ONLY) + (P_FKO_KNOWN);
[[maybe_unused]] constexpr double chk_P_POCKET_ONLINE = (P_POCKET_ONLINE);
[[maybe_unused]] constexpr double chk_P_POCKET_ONLINE_MAXLEN = (P_POCKET_ONLINE_MAXLEN);
[[maybe_unused]] constexpr double chk_P_POCKET_ONLINE_MIN_EXP = (P_POCKET_ONLINE_MIN_EXP);
[[maybe_unused]] constexpr double chk_P_ZONE_ONLINE = (P_ZONE_ONLINE);
[[maybe_unused]] constexpr double chk_P_ZONE_RICH_GAP = (P_ZONE_RICH_GAP);
[[maybe_unused]] constexpr double chk_P_ZONE_ONLINE_UNTIL = (P_ZONE_ONLINE_UNTIL);
[[maybe_unused]] constexpr double chk_P_GOSSIP_CD = (P_GOSSIP_CD);
[[maybe_unused]] constexpr double chk_P_RICH_GAP = (P_RICH_GAP);
[[maybe_unused]] constexpr double chk_P_RICH_AGE = (P_RICH_AGE);
[[maybe_unused]] constexpr double chk_P_RICH_MSGS = (P_RICH_MSGS);
[[maybe_unused]] constexpr double chk_P_SONAR_AIM = (P_SONAR_AIM);
[[maybe_unused]] constexpr double chk_P_GAP_CD = (P_GAP_CD);
[[maybe_unused]] constexpr double chk_P_PORTAL_PATHFIX = (P_PORTAL_PATHFIX);
[[maybe_unused]] constexpr double chk_P_EXPO_MINLEN = (P_EXPO_MINLEN);
[[maybe_unused]] constexpr double chk_P_EXPO_AGE = (P_EXPO_AGE);
[[maybe_unused]] constexpr double chk_P_FARM_VALUE = (P_FARM_VALUE);
[[maybe_unused]] constexpr double chk_P_FARM_GAMMA = (P_FARM_GAMMA);
[[maybe_unused]] constexpr double chk_P_PORTAL_TARGET = (P_PORTAL_TARGET);
[[maybe_unused]] constexpr double chk_P_PORTAL_EXPLORE = (P_PORTAL_EXPLORE);
[[maybe_unused]] constexpr double chk_P_GOSSIP = (P_GOSSIP);
[[maybe_unused]] constexpr double chk_P_W_PATH = (P_W_PATH);
[[maybe_unused]] constexpr double chk_P_W_DIFF = (P_W_DIFF);
[[maybe_unused]] constexpr double chk_P_BEAM_W = (P_BEAM_W);
[[maybe_unused]] constexpr double chk_P_BEAM_D = (P_BEAM_D);
[[maybe_unused]] constexpr double chk_P_BEAM_Q = (P_BEAM_Q);
[[maybe_unused]] constexpr double chk_P_BEAM_NODES = (P_BEAM_NODES);
[[maybe_unused]] constexpr double chk_P_PATH_GAMMA = (P_PATH_GAMMA);
[[maybe_unused]] constexpr double chk_P_TAIL_W = (P_TAIL_W);
[[maybe_unused]] constexpr double chk_P_PHI_KAPPA = (P_PHI_KAPPA);
[[maybe_unused]] constexpr double chk_P_PHI_CLUSTER = (P_PHI_CLUSTER);
[[maybe_unused]] constexpr double chk_P_GAMMA = (P_GAMMA);
[[maybe_unused]] constexpr double chk_P_BFS_RADIUS = (P_BFS_RADIUS);
[[maybe_unused]] constexpr double chk_P_COMP_ENEMY = (P_COMP_ENEMY) + (P_COMP_ENEMY_MAPV);
[[maybe_unused]] constexpr double chk_P_COMP_ALLY = (P_COMP_ALLY);
[[maybe_unused]] constexpr double chk_P_COMP_RADIUS = (P_COMP_RADIUS);
[[maybe_unused]] constexpr double chk_P_W_EAT = (P_W_EAT);
[[maybe_unused]] constexpr double chk_P_W_GOAL = (P_W_GOAL);
[[maybe_unused]] constexpr double chk_P_SURV_DEPTH = (P_SURV_DEPTH);
[[maybe_unused]] constexpr double chk_P_TURN_DFS_BUDGET = (P_TURN_DFS_BUDGET);
[[maybe_unused]] constexpr double chk_P_DFS_LIMIT = (P_DFS_LIMIT);
[[maybe_unused]] constexpr double chk_P_W_PESS = (P_W_PESS);
[[maybe_unused]] constexpr double chk_P_W_PESS_ALLY = (P_W_PESS_ALLY);
[[maybe_unused]] constexpr double chk_P_W_ALLY_TRAP = (P_W_ALLY_TRAP);
[[maybe_unused]] constexpr double chk_P_W_SPACE = (P_W_SPACE);
[[maybe_unused]] constexpr double chk_P_SPACE_LEN_SCALE = (P_SPACE_LEN_SCALE);
[[maybe_unused]] constexpr double chk_P_STRIKE_FAV = (P_STRIKE_FAV);
[[maybe_unused]] constexpr double chk_P_STRIKE_EQ = (P_STRIKE_EQ);
[[maybe_unused]] constexpr double chk_P_STRIKE_UNFAV = (P_STRIKE_UNFAV);
[[maybe_unused]] constexpr double chk_P_STRIKE_SPRINT = (P_STRIKE_SPRINT);
[[maybe_unused]] constexpr double chk_P_BIGDIFF = (P_BIGDIFF);
[[maybe_unused]] constexpr double chk_P_STRIKE_BIGDIFF = (P_STRIKE_BIGDIFF);
[[maybe_unused]] constexpr double chk_P_EQ_LOSS = (P_EQ_LOSS);
[[maybe_unused]] constexpr double chk_P_UNSEEN_REACH = (P_UNSEEN_REACH);
[[maybe_unused]] constexpr double chk_P_UNIT_VALUE = (P_UNIT_VALUE);
[[maybe_unused]] constexpr double chk_P_SPRINT_COST = (P_SPRINT_COST);
[[maybe_unused]] constexpr double chk_P_PORTAL_BLIND = (P_PORTAL_BLIND);
[[maybe_unused]] constexpr double chk_P_PORTAL_EXIT_STAY = (P_PORTAL_EXIT_STAY);
[[maybe_unused]] constexpr double chk_P_ENDGAME_ROUND = (P_ENDGAME_ROUND);
[[maybe_unused]] constexpr double chk_P_ENDGAME_RISK = (P_ENDGAME_RISK);
[[maybe_unused]] constexpr double chk_P_RESCUE_X = (P_RESCUE_X);
[[maybe_unused]] constexpr double chk_P_RESCUE_BLIND_TURNS = (P_RESCUE_BLIND_TURNS);
[[maybe_unused]] constexpr double chk_P_RESCUE_PARENT = (P_RESCUE_PARENT);
[[maybe_unused]] constexpr double chk_P_RESCUE_PARENT_MINLEN = (P_RESCUE_PARENT_MINLEN);
[[maybe_unused]] constexpr double chk_P_SPLIT_MIN = (P_SPLIT_MIN);
[[maybe_unused]] constexpr double chk_P_MAX_UNITS = (P_MAX_UNITS);
[[maybe_unused]] constexpr double chk_P_GROW_ROUND = (P_GROW_ROUND);
[[maybe_unused]] constexpr double chk_P_SPLIT_END = (P_SPLIT_END);
[[maybe_unused]] constexpr double chk_P_CHILD_MAX = (P_CHILD_MAX);
[[maybe_unused]] constexpr double chk_P_SPLIT_ENEMY_DIST = (P_SPLIT_ENEMY_DIST);
[[maybe_unused]] constexpr double chk_P_SPLIT_CROWD = (P_SPLIT_CROWD);
[[maybe_unused]] constexpr double chk_P_SPLIT_SPACE_MULT = (P_SPLIT_SPACE_MULT);
[[maybe_unused]] constexpr double chk_P_SPLIT_DENSITY = (P_SPLIT_DENSITY);
[[maybe_unused]] constexpr double chk_P_AREA_PER_UNIT = (P_AREA_PER_UNIT);
[[maybe_unused]] constexpr double chk_P_ATTACK_MIN_GAIN = (P_ATTACK_MIN_GAIN);
[[maybe_unused]] constexpr double chk_P_ATTACK_MAX_STEPS = (P_ATTACK_MAX_STEPS);
[[maybe_unused]] constexpr double chk_P_DEADEND = (P_DEADEND);
[[maybe_unused]] constexpr double chk_P_DEADEND_COST = (P_DEADEND_COST);
[[maybe_unused]] constexpr double chk_P_DEADEND_MIN_ROUND = (P_DEADEND_MIN_ROUND);
[[maybe_unused]] constexpr double chk_P_DEADEND_MINPEARLS = (P_DEADEND_MINPEARLS);
[[maybe_unused]] constexpr double chk_P_HUNT = (P_HUNT);
[[maybe_unused]] constexpr double chk_P_HUNT_AGE = (P_HUNT_AGE);
[[maybe_unused]] constexpr double chk_P_HUNT_MIN = (P_HUNT_MIN);
[[maybe_unused]] constexpr double chk_P_HUNT_MARGIN = (P_HUNT_MARGIN);
[[maybe_unused]] constexpr double chk_P_HUNT_MINUNITS = (P_HUNT_MINUNITS);
[[maybe_unused]] constexpr double chk_P_ATTACK_LOCAL = (P_ATTACK_LOCAL) + (P_ATTACK_LOCAL_MARGIN) + (P_ATTACK_LOCAL_R) + (P_ATTACK_LOCAL_WHERE);
[[maybe_unused]] constexpr double chk_P_OUTNUM = (P_OUTNUM_P) + (P_OUTNUM_R);
[[maybe_unused]] constexpr double chk_P_PORTALS_BL = (P_PORTALS_BLIND) + (P_PORTALS_EXIT_STAY);
[[maybe_unused]] constexpr double chk_P_REINFORCE = (P_REINFORCE);
[[maybe_unused]] constexpr double chk_P_ATTACK_UNFAV = (P_ATTACK_UNFAV) + (P_ATTACK_UNFAV_MARGIN);
[[maybe_unused]] constexpr double chk_P_KING_GUARD = (P_KING_GUARD) + (P_KING_GUARD_R) + (P_KING_GUARD_MAXLEN) + (P_KING_GUARD_PULL) + (P_KING_GUARD_MINKING);
[[maybe_unused]] constexpr double chk_P_OPEN_RUSH_SPREAD = (P_OPEN_RUSH_SPREAD);
[[maybe_unused]] constexpr double chk_P_KMEET = (P_KMEET) + (P_KMEET_MINLEN) + (P_KMEET_PULL) + (P_KMEET_ZONE);
[[maybe_unused]] constexpr double chk_P_SKIRMISH = (P_SKIRMISH) + (P_SKIRMISH_MAXLEN) + (P_SKIRMISH_EQ) + (P_SKIRMISH_EQ_MARGIN) + (P_SKIRMISH_WHERE);
[[maybe_unused]] constexpr double chk_P_HUNT_BODY = (P_HUNT_BODY);
[[maybe_unused]] constexpr double chk_P_HUNT_BODY_MIN = (P_HUNT_BODY_MIN);
[[maybe_unused]] constexpr double chk_P_KILL = (P_KILL);
[[maybe_unused]] constexpr double chk_P_KILL_WINDOW = (P_KILL_WINDOW);
[[maybe_unused]] constexpr double chk_P_KILL_MIN_ROUND = (P_KILL_MIN_ROUND);
[[maybe_unused]] constexpr double chk_P_KILL_MAX_ENEMY = (P_KILL_MAX_ENEMY);
[[maybe_unused]] constexpr double chk_P_KILL_RATIO = (P_KILL_RATIO);
[[maybe_unused]] constexpr double chk_P_HUNT_KILL = (P_HUNT_KILL);
[[maybe_unused]] constexpr double chk_P_KING = (P_KING);
[[maybe_unused]] constexpr double chk_P_KING_SPLIT_UNTIL = (P_KING_SPLIT_UNTIL);
[[maybe_unused]] constexpr double chk_P_KING_CLAIM_LEN = (P_KING_CLAIM_LEN);
[[maybe_unused]] constexpr double chk_P_KING_FRESH = (P_KING_FRESH);
[[maybe_unused]] constexpr double chk_P_KING_RELAY_AGE = (P_KING_RELAY_AGE);
[[maybe_unused]] constexpr double chk_P_KING_RISK = (P_KING_RISK);
[[maybe_unused]] constexpr double chk_P_KING_STRIKE = (P_KING_STRIKE);
[[maybe_unused]] constexpr double chk_P_KING_SURV_DEPTH = (P_KING_SURV_DEPTH);
[[maybe_unused]] constexpr double chk_P_KING_DFS_LIMIT = (P_KING_DFS_LIMIT);
[[maybe_unused]] constexpr double chk_P_LONG_DFS_LIMIT = (P_LONG_DFS_LIMIT);
[[maybe_unused]] constexpr double chk_P_LONG_TRAP = (P_LONG_TRAP);
[[maybe_unused]] constexpr double chk_P_LONG_TRAP_LEN = (P_LONG_TRAP_LEN);
[[maybe_unused]] constexpr double chk_P_TAIL_SLACK = (P_TAIL_SLACK);
[[maybe_unused]] constexpr double chk_P_ROOM_FACTOR = (P_ROOM_FACTOR);
[[maybe_unused]] constexpr double chk_P_ROOM_MINLEN = (P_ROOM_MINLEN);
[[maybe_unused]] constexpr double chk_P_BODY_MSG = (P_BODY_MSG);
[[maybe_unused]] constexpr double chk_P_POCKET = (P_POCKET);
[[maybe_unused]] constexpr double chk_P_POCKET_MAXLEN = (P_POCKET_MAXLEN);
[[maybe_unused]] constexpr double chk_P_POCKET_MIN_EXP = (P_POCKET_MIN_EXP);
[[maybe_unused]] constexpr double chk_P_POCKET_COST = (P_POCKET_COST);
[[maybe_unused]] constexpr double chk_P_POCKET_MIN_ROUND = (P_POCKET_MIN_ROUND);
[[maybe_unused]] constexpr double chk_P_POCKET_PULL = (P_POCKET_PULL);
[[maybe_unused]] constexpr double chk_P_POCKET_CONT = (P_POCKET_CONT);
[[maybe_unused]] constexpr double chk_P_ROLE_FEED_START = (P_ROLE_FEED_START);
[[maybe_unused]] constexpr double chk_P_DB_TENTATIVE = (P_DB_TENTATIVE);
[[maybe_unused]] constexpr double chk_P_COURIER_LEN = (P_COURIER_LEN);
[[maybe_unused]] constexpr double chk_P_FARMER_KEEP = (P_FARMER_KEEP);
[[maybe_unused]] constexpr double chk_P_COURIER_PULL = (P_COURIER_PULL);
[[maybe_unused]] constexpr double chk_P_FARMERS = (P_FARMERS);
[[maybe_unused]] constexpr double chk_P_COURIER_KINGMAX = (P_COURIER_KINGMAX);
[[maybe_unused]] constexpr double chk_P_KING_CAP = (P_KING_CAP);
[[maybe_unused]] constexpr double chk_P_LATE_KING = (P_LATE_KING);
[[maybe_unused]] constexpr double chk_P_CUT_TRAP = (P_CUT_TRAP);
[[maybe_unused]] constexpr double chk_P_LONG_ALLMAPS = (P_LONG_ALLMAPS);
[[maybe_unused]] constexpr double chk_P_KING_CAP_UNTIL = (P_KING_CAP_UNTIL);
[[maybe_unused]] constexpr double chk_P_ZONE_MARGIN = (P_ZONE_MARGIN);
[[maybe_unused]] constexpr double chk_P_ZONE_MINDIST = (P_ZONE_MINDIST);
[[maybe_unused]] constexpr double chk_P_ZONE_D0 = (P_ZONE_D0);
[[maybe_unused]] constexpr double chk_P_ZONE_UNTIL = (P_ZONE_UNTIL);
[[maybe_unused]] constexpr double chk_P_ZONE_MIN_RATE = (P_ZONE_MIN_RATE);
[[maybe_unused]] constexpr double chk_P_ZONE_MIN_RATIO = (P_ZONE_MIN_RATIO);
[[maybe_unused]] constexpr double chk_P_ZONE_RADIUS = (P_ZONE_RADIUS);
[[maybe_unused]] constexpr double chk_P_ZONE_PULL = (P_ZONE_PULL);
[[maybe_unused]] constexpr double chk_P_COURIER_BLIND_UNTIL = (P_COURIER_BLIND_UNTIL);
[[maybe_unused]] constexpr double chk_P_KING_DANGER = (P_KING_DANGER);
[[maybe_unused]] constexpr double chk_P_DANGER_AGE = (P_DANGER_AGE);
[[maybe_unused]] constexpr double chk_P_KING_AWAY = (P_KING_AWAY);
[[maybe_unused]] constexpr double chk_P_KING_DEFER = (P_KING_DEFER);
[[maybe_unused]] constexpr double chk_P_DEFER_RADIUS = (P_DEFER_RADIUS);
[[maybe_unused]] constexpr double chk_P_DEFER_SLACK = (P_DEFER_SLACK);
[[maybe_unused]] constexpr double chk_P_KING_SPACE = (P_KING_SPACE);
[[maybe_unused]] constexpr double chk_P_KING_LEN = (P_KING_LEN);
[[maybe_unused]] constexpr double chk_P_KING_NOSPLIT = (P_KING_NOSPLIT);
[[maybe_unused]] constexpr double chk_P_FEED_START = (P_FEED_START);
[[maybe_unused]] constexpr double chk_P_FEED_PULL_START = (P_FEED_PULL_START);
[[maybe_unused]] constexpr double chk_P_FEED_PULL = (P_FEED_PULL);
[[maybe_unused]] constexpr double chk_P_FEED_MINUNITS = (P_FEED_MINUNITS);
[[maybe_unused]] constexpr double chk_P_FEED_MARGIN = (P_FEED_MARGIN);
[[maybe_unused]] constexpr double chk_P_FEED_ATTRACT = (P_FEED_ATTRACT);
[[maybe_unused]] constexpr double chk_P_DONATE_DIST = (P_DONATE_DIST);
[[maybe_unused]] constexpr double chk_P_DONATE_MAXPEARLS = (P_DONATE_MAXPEARLS);
[[maybe_unused]] constexpr double chk_P_E2_FEED = (P_E2_FEED);
[[maybe_unused]] constexpr double chk_P_E2_PULL = (P_E2_PULL);
[[maybe_unused]] constexpr double chk_P_E2_GROW = (P_E2_GROW);
[[maybe_unused]] constexpr double chk_P_E2_KING_SAFE = (P_E2_KING_SAFE);
[[maybe_unused]] constexpr double chk_P_E2_LEAD = (P_E2_LEAD);
[[maybe_unused]] constexpr double chk_P_E2_KING_NOATTACK = (P_E2_KING_NOATTACK);
[[maybe_unused]] constexpr double chk_P_E2_LOCALMIN = (P_E2_LOCALMIN) + (P_LOCALMIN_LEN);
[[maybe_unused]] constexpr double chk_P_E2_LONGSAFE = (P_E2_LONGSAFE);
[[maybe_unused]] constexpr double chk_P_E2_HORIZON = (P_E2_HORIZON) + (P_E2_HORIZON_MARGIN);
[[maybe_unused]] constexpr double chk_P_E2_KING_FRESH = (P_E2_KING_FRESH);
[[maybe_unused]] constexpr double chk_P_E2_HUNT = (P_E2_HUNT);
[[maybe_unused]] constexpr double chk_P_E2_DUMP_ROUND = (P_E2_DUMP_ROUND);
[[maybe_unused]] constexpr double chk_P_RV = (P_RV);
[[maybe_unused]] constexpr double chk_P_E2_PATHDIST = (P_E2_PATHDIST);
[[maybe_unused]] constexpr double chk_P_RV_LEAD = (P_RV_LEAD);
[[maybe_unused]] constexpr double chk_P_RV_SEARCH = (P_RV_SEARCH);
[[maybe_unused]] constexpr double chk_P_RV_ZONE = (P_RV_ZONE);
[[maybe_unused]] constexpr double chk_P_RV_PULL = (P_RV_PULL);
[[maybe_unused]] constexpr double chk_P_RV_MINTARGET = (P_RV_MINTARGET);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_START = (P_E2_HUNT_START);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_MINLEN = (P_E2_HUNT_MINLEN);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_MAXLEN = (P_E2_HUNT_MAXLEN);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_FRAC = (P_E2_HUNT_FRAC);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_AGE = (P_E2_HUNT_AGE);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_TGT = (P_E2_HUNT_TGT);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_MARGIN = (P_E2_HUNT_MARGIN);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_ATTRACT = (P_E2_HUNT_ATTRACT);
[[maybe_unused]] constexpr double chk_P_E2_HUNT_PULL = (P_E2_HUNT_PULL);
[[maybe_unused]] constexpr double chk_P_LOCAL_FEED_DIST = (P_LOCAL_FEED_DIST);
[[maybe_unused]] constexpr double chk_P_CAP_FEED = (P_CAP_FEED);
[[maybe_unused]] constexpr double chk_P_CAP_FEED_MARGIN = (P_CAP_FEED_MARGIN);
[[maybe_unused]] constexpr double chk_P_CAP_FEED_MINLEN = (P_CAP_FEED_MINLEN);
[[maybe_unused]] constexpr double chk_P_BIG_LEAD = (P_BIG_LEAD);
[[maybe_unused]] constexpr double chk_P_BIG_MINLEN = (P_BIG_MINLEN);
[[maybe_unused]] constexpr double chk_P_ISO_ROUNDS = (P_ISO_ROUNDS);
[[maybe_unused]] constexpr double chk_P_ISO_MIN_ROUND = (P_ISO_MIN_ROUND);
[[maybe_unused]] constexpr double chk_P_ISO_FEED_START = (P_ISO_FEED_START);
[[maybe_unused]] constexpr double chk_P_SPACE_ALLY = (P_SPACE_ALLY);
[[maybe_unused]] constexpr double chk_P_TRAUMA_PLAN = (P_TRAUMA_PLAN);
[[maybe_unused]] constexpr double chk_P_SCH_ZONE_PULL = (P_SCH_ZONE_PULL);
[[maybe_unused]] constexpr double chk_P_SCH_ZONE_FRAC = (P_SCH_ZONE_FRAC);
[[maybe_unused]] constexpr double chk_P_SCH_ZONE_UNTIL = (P_SCH_ZONE_UNTIL);
[[maybe_unused]] constexpr double chk_P_DEVIL_UNSEEN = (P_DEVIL_UNSEEN);
[[maybe_unused]] constexpr double chk_P_QUEEN_UNSEEN = (P_QUEEN_UNSEEN);
[[maybe_unused]] constexpr double chk_P_DEFAULT_UNSEEN = (P_DEFAULT_UNSEEN);
[[maybe_unused]] constexpr double chk_P_DILEMMA_UNSEEN = (P_DILEMMA_UNSEEN);
[[maybe_unused]] constexpr double chk_P_AUTARKY_UNSEEN = (P_AUTARKY_UNSEEN);
[[maybe_unused]] constexpr double chk_P_DSMALL_UNSEEN = (P_DSMALL_UNSEEN);
[[maybe_unused]] constexpr double chk_P_TROPHY_UNSEEN = (P_TROPHY_UNSEEN);
[[maybe_unused]] constexpr double chk_P_TROPHY_ZONE_PULL = (P_TROPHY_ZONE_PULL);
[[maybe_unused]] constexpr double chk_P_TROPHY_ZONE_FRAC = (P_TROPHY_ZONE_FRAC);
[[maybe_unused]] constexpr double chk_P_TROPHY_ZONE_UNTIL = (P_TROPHY_ZONE_UNTIL);
[[maybe_unused]] constexpr double chk_P_PORTALS_FEED = (P_PORTALS_FEED);
[[maybe_unused]] constexpr double chk_P_PORTALS_PULL = (P_PORTALS_PULL);
[[maybe_unused]] constexpr double chk_P_PORTALS_RV = (P_PORTALS_RV);
[[maybe_unused]] constexpr double chk_P_DB_VARIANT = (P_DB_VARIANT);
[[maybe_unused]] constexpr double chk_P_MAPDUMP = (P_SCH_DUMP) + (P_SLI_DUMP) + (P_DEFAULT_DUMP) + (P_DEFAULT_GROW) + (P_DEFAULT_FEED) + (P_DEFAULT_PULL) + (P_AUTARKY_POCKET) + (P_DILEMMA_POCKET);
[[maybe_unused]] constexpr double chk_P_KSM = (P_KING_SPACE_MARGIN) + (P_KING_SPACE_MARGIN_LEN);
[[maybe_unused]] constexpr double chk_P_PORTALS_KRISK = (P_PORTALS_KRISK) + (P_PORTALS_KRISK_END);
[[maybe_unused]] constexpr double chk_P_INVADE = (P_INVADE_FRAC) + (P_INVADE_START) + (P_INVADE_PULL) + (P_INVADE_ARRIVE) + (P_HOME_GUARD);
[[maybe_unused]] constexpr double chk_P_HEIR = (P_HEIR) + (P_HEIR_MIN) + (P_HEIR_START) + (P_HEIR_FRAC);
[[maybe_unused]] constexpr double chk_P_GROWU = (P_GROW_UNITS) + (P_SECTOR) + (P_SECTOR_SIZE) + (P_SECTOR_PULL) + (P_SECTOR_START) + (P_SECTOR_UNTIL) + (P_SECTOR_MAXLEN);
[[maybe_unused]] constexpr double chk_P_HOLD = (P_HOLD) + (P_HOLD_FRAC) + (P_HOLD_PULL) + (P_HOLD_STAY) + (P_HOLD_START) + (P_HOLD_UNTIL) + (P_HOLD_MAXLEN) + (P_RUSH_NOTRADE);
[[maybe_unused]] constexpr double chk_P_GATE = (P_KING_GATE) + (P_KG_START) + (P_KG_UNTIL) + (P_KG_PULL) + (P_KG_BONUS) + (P_SURV_MAYBE) + (P_SURV_MAYBE_LEN) + (P_SURV_MAYBE_WHERE) + (P_FENCE_SWARM) + (P_FENCE_START) + (P_GATE_DOOR) + (P_INVADE_DOOR) + (P_GATE_SWARM) + (P_TRAUMA_GATE) + (P_GATE_START) + (P_GATE_R) + (P_GATE_PULL) + (P_GATE_MINDIST);
[[maybe_unused]] constexpr double chk_P_KDROP = (P_KING_DROP_PEN) + (P_KING_DROP_R);
[[maybe_unused]] constexpr double chk_P_ESPLIT = (P_ELIM_SPLIT);
[[maybe_unused]] constexpr double chk_P_GRAD = (P_GRAD_FEED) + (P_GRAD_FEED_MINLEN);
[[maybe_unused]] constexpr double chk_P_PSPRINT = (P_PSPRINT) + (P_PSPRINT_MAX) + (P_PSPRINT_WHERE);
[[maybe_unused]] constexpr double chk_P_SPREAD = (P_SPREAD_W) + (P_SPREAD_R);
[[maybe_unused]] constexpr double chk_P_DB_EXTRA = (P_DB_EXTRA_MAX) + (P_DB_EXTRA_A) + (P_DB_EXTRA_B);
[[maybe_unused]] constexpr double chk_P_KING_ANCHOR_R = (P_KING_ANCHOR_R);
[[maybe_unused]] constexpr double chk_P_KING_ANCHOR_PULL = (P_KING_ANCHOR_PULL);
[[maybe_unused]] constexpr double chk_P_ROLE_WAIT_DB = (P_ROLE_WAIT_DB);
[[maybe_unused]] constexpr double chk_P_UNIT_FENCE = (P_UNIT_FENCE);
[[maybe_unused]] constexpr double chk_P_MAP_GOSSIP = (P_MAP_GOSSIP);
[[maybe_unused]] constexpr double chk_P_KING_REPLACE = (P_KING_REPLACE);
[[maybe_unused]] constexpr double chk_P_PORTAL_TOP_BLOCK = (P_PORTAL_TOP_BLOCK);
[[maybe_unused]] constexpr double chk_P_ATTACK_LONG_MIN = (P_ATTACK_LONG_MIN);
[[maybe_unused]] constexpr double chk_P_PORTAL_RISK = (P_PORTAL_RISK);
[[maybe_unused]] constexpr double chk_P_PORTAL_RISK_LEN = (P_PORTAL_RISK_LEN);
[[maybe_unused]] constexpr double chk_P_PORTAL_RISK_KING = (P_PORTAL_RISK_KING);
[[maybe_unused]] constexpr double chk_P_DB_SPAWNS = (P_DB_SPAWNS);
[[maybe_unused]] constexpr double chk_P_ATTEMPT_MEMORY = (P_ATTEMPT_MEMORY);
[[maybe_unused]] constexpr double chk_P_HOME_WALLS = (P_HOME_WALLS);
[[maybe_unused]] constexpr double chk_P_ATTEMPT_SIDE = (P_ATTEMPT_SIDE);
[[maybe_unused]] constexpr double chk_P_ATTEMPT_OWN_CLASS = (P_ATTEMPT_OWN_CLASS);
[[maybe_unused]] constexpr double chk_P_ATTEMPT_MAXGAP = (P_ATTEMPT_MAXGAP);
[[maybe_unused]] constexpr double chk_P_ATTEMPT_ENCLOSED = (P_ATTEMPT_ENCLOSED);
[[maybe_unused]] constexpr double chk_P_DOOM_STRICT_LEN = (P_DOOM_STRICT_LEN);
[[maybe_unused]] constexpr double chk_P_DOOM_SURV = (P_DOOM_SURV);
[[maybe_unused]] constexpr double chk_P_RESCUE_STRICT_LEN = (P_RESCUE_STRICT_LEN);
[[maybe_unused]] constexpr double chk_P_RESCUE_SURV = (P_RESCUE_SURV);
[[maybe_unused]] constexpr double chk_P_BIG_NOSPLIT_LEN = (P_BIG_NOSPLIT_LEN);
[[maybe_unused]] constexpr double chk_P_SMALL_STRIKER_LEN = (P_SMALL_STRIKER_LEN);
[[maybe_unused]] constexpr double chk_P_SMALL_TARGET_LEN = (P_SMALL_TARGET_LEN);
[[maybe_unused]] constexpr double chk_P_SMALL_STRIKE = (P_SMALL_STRIKE);
[[maybe_unused]] constexpr double chk_P_SURV_FLOOR = (P_SURV_FLOOR);
[[maybe_unused]] constexpr double chk_P_BUDGET_64_BIG = (P_BUDGET_64_BIG);
[[maybe_unused]] constexpr double chk_P_SIDE_DETECT = (P_SIDE_DETECT);
[[maybe_unused]] constexpr double chk_P_TAIL_GROW = (P_TAIL_GROW) + (P_TAIL_GROW_KNOWN);
[[maybe_unused]] constexpr double chk_P_TAIL_SPLIT = (P_TAIL_SPLIT);
[[maybe_unused]] constexpr double chk_P_KING_PESS2 = (P_KING_PESS2);
[[maybe_unused]] constexpr double chk_P_KING_SPACE2 = (P_KING_SPACE2);
[[maybe_unused]] constexpr double chk_P_KING_SPACE_ALLY = (P_KING_SPACE_ALLY);
[[maybe_unused]] constexpr double chk_P_DEADEND_FLOOR = (P_DEADEND_FLOOR);
[[maybe_unused]] constexpr double chk_P_BOX_ONEWAY = (P_BOX_ONEWAY);
[[maybe_unused]] constexpr double chk_P_LANDING_PEN = (P_LANDING_PEN);
[[maybe_unused]] constexpr double chk_P_LONG_STRIKE_LEN = (P_LONG_STRIKE_LEN);
[[maybe_unused]] constexpr double chk_P_LONG_STRIKE = (P_LONG_STRIKE);
[[maybe_unused]] constexpr double chk_P_BIG_SPRINT_FULL = (P_BIG_SPRINT_FULL);
[[maybe_unused]] constexpr double chk_P_PORTAL_PROBE = (P_PORTAL_PROBE);
[[maybe_unused]] constexpr double chk_P_PROBE_MAXLEN = (P_PROBE_MAXLEN);
[[maybe_unused]] constexpr double chk_P_PROBE_PEN = (P_PROBE_PEN);
[[maybe_unused]] constexpr double chk_P_MIRROR_FIX = (P_MIRROR_FIX);
[[maybe_unused]] constexpr double chk_P_PREV_FIX = (P_PREV_FIX) + (P_ZONE_POCKETS);
[[maybe_unused]] constexpr double chk_P_LONE = (P_LONE_HUNT) + (P_LONE_START) + (P_LONE_MAXLEN) + (P_LONE_MINUNITS) + (P_LONE_WINDOW) + (P_LONE_MAX) + (P_LONE_MINLEN) + (P_LONE_PULL);
[[maybe_unused]] constexpr double chk_P_OPEN_RUSH = (P_OPEN_RUSH) + (P_OPEN_RUSH_N) + (P_OPEN_RUSH_UNTIL) + (P_OPEN_RUSH_ARRIVE);
[[maybe_unused]] constexpr double chk_P_MIRROR_UNSEEN_P = (P_MIRROR_UNSEEN_P);
[[maybe_unused]] constexpr double chk_P_E7_DEFER = (P_E7_DEFER);
[[maybe_unused]] constexpr double chk_P_E7_DEFER_F = (P_E7_DEFER_F);
[[maybe_unused]] constexpr double chk_P_E7_DUMP = (P_E7_DUMP) + (P_PORTALS_DUMP) + (P_PORTALS_FKO) + (P_PORTALS_GROW);
[[maybe_unused]] constexpr double chk_P_E7_DUMP_MARGIN = (P_E7_DUMP_MARGIN);
[[maybe_unused]] constexpr double chk_P_E7_BIG_LEN = (P_E7_BIG_LEN);
[[maybe_unused]] constexpr double chk_P_E7_BIG_FEED = (P_E7_BIG_FEED);
[[maybe_unused]] constexpr double chk_P_E7_BIG_PULL = (P_E7_BIG_PULL);
[[maybe_unused]] constexpr double chk_P_ELIM_FEED = (P_ELIM_FEED);
[[maybe_unused]] constexpr double chk_P_ELIM_PULL = (P_ELIM_PULL);
[[maybe_unused]] constexpr double chk_P_DEVIL_ZONE_PULL = (P_DEVIL_ZONE_PULL);
[[maybe_unused]] constexpr double chk_P_DEVIL_ZONE_FRAC = (P_DEVIL_ZONE_FRAC);
[[maybe_unused]] constexpr double chk_P_DEVIL_ZONE_UNTIL = (P_DEVIL_ZONE_UNTIL);
}

namespace {

// ------------------------------------------------------------------ basics
constexpr int MAXN = 256 * 256;   // the engine accepts maps from 7x7 up to 256x256
// Per-tile arrays are allocated for the actual map size once it is known (a 256x256 map needs
// ~30 MB, a 32x32 map ~0.5 MB; every dragon is its own process, so this matters).
int allocN = 0;   // tiles allocated for (0 = not yet)
std::vector<std::function<void()>>& allocList() { static std::vector<std::function<void()>> v; return v; }
template <class T> struct TileArr {
    T* p = nullptr; int mult; T init;
    explicit TileArr(int m = 1, T i = T()) : mult(m), init(i) {
        auto f = [this]() {
            delete[] p;
            size_t n = (size_t)allocN * mult;
            // zero-initialised arrays are cleared in bulk (memset); only others are filled one by one
            static const T zero{};
            if (std::memcmp(&init, &zero, sizeof(T)) == 0) p = new T[n]();
            else { p = new T[n]; std::fill(p, p + n, init); }
        };
        allocList().push_back(f);
        if (allocN > 0) f();   // (function-local arrays created after the map size is known)
    }
    TileArr(const TileArr&) = delete;
    inline T& operator[](size_t i) { return p[i]; }
    inline const T& operator[](size_t i) const { return p[i]; }
};
void allocTiles(int n) {
    if (n == allocN) return;
    allocN = n;
    for (auto& f : allocList()) f();
}
constexpr int BIG_FREE = 400;  // "never frees within horizon"
constexpr int16_t UNR = INT16_MAX;
constexpr int16_t E_UNK = -3, E_KELP = -2, E_OPEN = -1;

int W, H, N, UNIT_LIMIT = 64;
int myId;
char myTeam;
int rnd = 0, myLen = 3, unitCount = 1, myFacing = 0;
int turnCount = 0;
int firstRound = -1;
std::vector<uint64_t> msgs;
int echoes[5];
// v8 portal probe: a sonar ray sent through a blind portal at the end of my turn tells me next turn whether
// the small room behind it is occupied (the ray stops at the first dragon it meets)
int probeDir = -1, probeFrom = -1, probeRound = -9, probeHit = -1;

const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
const char DCH[4] = {'N', 'E', 'S', 'W'};

inline int wx(int x) { x %= W; return x < 0 ? x + W : x; }
inline int wy(int y) { y %= H; return y < 0 ? y + H : y; }
inline int ID(int x, int y) { return wy(y) * W + wx(x); }
inline int TX(int t) { return t % W; }
inline int TY(int t) { return t / W; }
inline int dirOf(char c) { return c == 'N' ? 0 : c == 'E' ? 1 : c == 'S' ? 2 : 3; }
inline int torusDX(int a, int b) { int d = std::abs(TX(a) - TX(b)); return std::min(d, W - d); }
inline int torusDY(int a, int b) { int d = std::abs(TY(a) - TY(b)); return std::min(d, H - d); }
inline int manhattan(int a, int b) { return torusDX(a, b) + torusDY(a, b); }
inline int cheb(int a, int b) { return std::max(torusDX(a, b), torusDY(a, b)); }

using Clock = std::chrono::steady_clock;
Clock::time_point turnStart;
#ifdef DEBUGLOG
Clock::time_point profT[16]; const char* profN[16]; int profK = 0;
#define PROF(name) do { if (profK < 16) { profT[profK] = Clock::now(); profN[profK++] = name; } } while (0)
#else
#define PROF(name) do {} while (0)
#endif
[[maybe_unused]] inline double elapsedMs() {
    return std::chrono::duration<double, std::milli>(Clock::now() - turnStart).count();
}

// ------------------------------------------------------------------ input
char lineBuf[1 << 16];
char* tok[16];
int ntok;

bool readLine() {
    while (fgets(lineBuf, sizeof lineBuf, stdin)) {
        char* p = lineBuf;
        ntok = 0;
        while (true) {
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
            if (!*p) break;
            if (ntok < 16) tok[ntok++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') ++p;
            if (*p) { *p = 0; ++p; }
        }
        if (ntok > 0) return true;
    }
    return false;
}

struct VTile { int x, y, pearl, cd; };
struct VPart { char team; int id, x, y, f; bool head; };
VTile vtile[49];
std::vector<VPart> vparts;
int16_t hrow[8][7], vrow[7][8];

int16_t edgeTok(const char* s) {
    if (s[0] == '.') return E_OPEN;
    if (s[0] == 'w') return E_KELP;
    return (int16_t)atoi(s);
}

bool readInit() {
    while (readLine()) {
        if (!strcmp(tok[0], "ID")) myId = atoi(tok[1]);
        else if (!strcmp(tok[0], "TEAM")) myTeam = tok[1][0];
        else if (!strcmp(tok[0], "MAP")) { W = atoi(tok[1]); H = atoi(tok[2]); }
        else if (!strcmp(tok[0], "UNIT_LIMIT")) { UNIT_LIMIT = atoi(tok[1]); return true; }
    }
    return false;
}

bool readRound() {
    if (!readLine()) return false;
    if (strcmp(tok[0], "ROUND") != 0) return false;
    rnd = atoi(tok[1]);
    readLine(); myFacing = dirOf(tok[1][0]);
    readLine(); myLen = atoi(tok[1]);
    readLine(); unitCount = atoi(tok[1]);
    readLine();
    int nm = atoi(tok[1]);
    msgs.clear();
    for (int i = 0; i < nm; i++) { readLine(); msgs.push_back(strtoull(tok[0], nullptr, 10)); }
    readLine();
    memset(echoes, 0, sizeof echoes);
    if (!strcmp(tok[0], "ECHOES")) {
        for (int i = 0; i < 5; i++) echoes[i] = atoi(tok[1 + i]);
        readLine();
    }
    for (int k = 0; k < 49; k++) {
        if (k > 0) readLine();
        vtile[k] = {atoi(tok[0]), atoi(tok[1]), atoi(tok[2]), std::min(atoi(tok[3]), 30000)};   // countdowns are kept in 16 bits
    }
    readLine();
    int nb = atoi(tok[1]);
    vparts.clear();
    for (int i = 0; i < nb; i++) {
        readLine();
        vparts.push_back({tok[0][0], atoi(tok[1]), atoi(tok[2]), atoi(tok[3]), dirOf(tok[4][0]), tok[5][0] == '1'});
    }
    for (int r = 0; r < 8; r++) { readLine(); for (int c = 0; c < 7; c++) hrow[r][c] = edgeTok(tok[c]); }
    for (int r = 0; r < 7; r++) { readLine(); for (int c = 0; c < 8; c++) vrow[r][c] = edgeTok(tok[c]); }
    return true;
}

// ------------------------------------------------------------------ output
std::string out;
void emit(const char* s) { out += s; out += '\n'; }

// ------------------------------------------------------------------ knowledge
struct TileK {
    int16_t cdR = -1, cdV = 0;      // countdown knowledge (direct or mirrored): value cdV at round cdR
    int16_t dR = -1, dV = 0;        // last direct countdown observation (for symmetry checks)
    int16_t pR = -1;                // last direct observation round (pearl state)
    bool pV = false;                // pearl present at pR
    int16_t gN = 0, gMin = 30000, gMax = 0;
    int16_t cdMax = -1;             // largest countdown ever observed (a lower bound of the spawn gap)
    int16_t aR = -1;                // v7.1: last spawn attempt known to have happened (kept when the countdown restarts)
    bool aMir = false;              // ...known only from the mirrored tile
    int32_t gSum = 0;
};
TileArr<TileK> tk;
// v7.1: before a countdown is overwritten, remember the attempt it announced if that round has come: a
// pearl appeared then (if the tile was free), whatever the new countdown says about the next one
inline void noteAttempt(TileK& K, int now, bool mirror = false) {
    if (K.cdR >= 0 && K.cdV >= 0) { int S = K.cdR + K.cdV; if (S <= now && S > K.aR) { K.aR = (int16_t)S; K.aMir = mirror; } }
}
TileArr<int16_t> eH(1, E_UNK), eV(1, E_UNK);   // north edge of tile, west edge of tile
TileArr<bool> eHd, eVd;             // directly observed
struct PortalEnd { int id; int code; };  // code = isV*MAXN + tile
std::vector<PortalEnd> portals;
bool symAlive[3] = {true, true, true};  // 0: mirror x, 1: mirror y, 2: rotate 180
int symKnown = -1;

int mirrorT(int t, int k) {
    int x = TX(t), y = TY(t);
    if (k == 0) return ID(W - 1 - x, y);
    if (k == 1) return ID(x, H - 1 - y);
    return ID(W - 1 - x, H - 1 - y);
}
int mirrorHE(int t, int k) {
    int x = TX(t), y = TY(t);
    if (k == 0) return ID(W - 1 - x, y);
    if (k == 1) return ID(x, H - y);
    return ID(W - 1 - x, H - y);
}
int mirrorVE(int t, int k) {
    int x = TX(t), y = TY(t);
    if (k == 0) return ID(W - x, y);
    if (k == 1) return ID(x, H - 1 - y);
    return ID(W - x, H - 1 - y);
}
inline int kindOf(int16_t e) { return e == E_KELP ? 1 : e >= 0 ? 2 : e == E_OPEN ? 0 : -1; }

bool nbFull = true;             // neighbour table needs a full rebuild
std::vector<int> nbDirty;       // tiles whose neighbour entries need refresh
void addPortal(int id, int code) {
    for (auto& p : portals) if (p.code == code) { if (p.id != id) { p.id = id; nbFull = true; } return; }
    portals.push_back({id, code});
    nbFull = true;
}
inline void setEH(int t, int16_t v) {
    if (eH[t] == v) return;
    eH[t] = v;
    nbDirty.push_back(t); nbDirty.push_back(ID(TX(t), TY(t) - 1));
}
inline void setEV(int t, int16_t v) {
    if (eV[t] == v) return;
    eV[t] = v;
    nbDirty.push_back(t); nbDirty.push_back(ID(TX(t) - 1, TY(t)));
}
int portalPartner(int id, int code) {
    for (auto& p : portals) if (p.id == id && p.code != code) return p.code;
    return -1;
}

// ------------------------------------------------------------------ embedded map database
int dbMap = -1;                 // index into MAPDB of the recognised map, -1 = unknown map
// v11: how other dragons' eating is modelled (P_TAIL_GROW): known maps and unknown maps separately
inline int tailGrowMode() { return dbMap >= 0 ? P_TAIL_GROW_KNOWN : P_TAIL_GROW; }
int dbWallsOf = -1;   // v7: known map whose walls/portals/strategy are kept although its pearl layout differs
int dbMap_() { return dbMap; }
bool dbTried = false;
int gapFor = -1;                // map whose gaps are loaded in gapA/gapB
TileArr<int16_t> gapA, gapB;  // spawn gap range per tile (only when dbMap >= 0); gapB == 0: never spawns

// ------------------------------------------------------------------ per-map strategy
struct Cfg {
    const char* name;
    int splitMin, maxUnits, kingSplitUntil, growRound, feedStart, feedPullStart;
    double hunt, kingDefer, deadEndCost;
    int unseenEarly;
    int pocketMaxLen;   // farm dead-end pockets with units up to this long (0 = never)
    double zonePull, zoneFrac;   // v6: rush the map's richest zone (known maps; 0 = P_ZONE_PULL)
    int zoneUntil;               // ...until this round
    int rv;                      // v6: endgame meeting tile (0 = P_RV; 1 = chosen by the king, 2 = our first spawn tile)
};
const Cfg CFG_DEFAULT = {"*", P_SPLIT_MIN, P_MAX_UNITS, P_KING_SPLIT_UNTIL, P_GROW_ROUND, P_FEED_START, P_FEED_PULL_START, P_HUNT, P_KING_DEFER, P_DEADEND_COST, P_UNSEEN_EARLY, P_POCKET_MAXLEN, 0.0, 0.0, 0, 0};
const Cfg CFG_TABLE[] = { P_MAP_TABLE };
// unknown maps: small and medium maps usually end in an elimination fight (pure swarm, no king
// until the very end); big maps go the distance (swarm + a king from round P_KING_SPLIT_UNTIL)
const Cfg CFG_UNKNOWN_SMALL = {"*small", P_SPLIT_MIN, P_MAX_UNITS, P_SMALL_KING_UNTIL, P_SMALL_GROW_ROUND, P_SMALL_FEED_START, P_SMALL_FEED_PULL_START, P_HUNT, 1.0, P_DEADEND_COST, P_UNSEEN_EARLY, P_POCKET_MAXLEN, 0.0, 0.0, 0, 0};
const Cfg& cfgBase(int m) {
    if (m < 0) return (P_ADAPT_CFG && N > 0 && N < P_BIG_MAP_TILES) ? CFG_UNKNOWN_SMALL : CFG_DEFAULT;
    for (const Cfg& c : CFG_TABLE) if (!strcmp(c.name, MAPDB[m].name)) return c;
    return CFG_DEFAULT;
}
// v5: the endgame consolidation starts no later than P_E2_PULL / P_E2_FEED on every map that
// consolidates at the end (the special early-king plans, feedStart < 300, are left alone)
Cfg cfgFor(int m) {
    Cfg c = cfgBase(m);
    // v17: unknown maps: an earlier endgame (a round-500 game is won by the longest dragon; on the ladder our
    // blind build lost on length with more total length than the other team: 165 to 95, 193 to 145, 63 to 29)
    if (m < 0 && P_UNK_FEED > 0) {
        c.feedStart = std::min(c.feedStart, (int)P_UNK_FEED);
        c.feedPullStart = std::min(c.feedPullStart, (int)P_UNK_PULL);
        c.growRound = std::min(c.growRound, (int)P_UNK_GROW);
    }
    if (m < 0 && P_UNK_KING_UNTIL < 9999) c.kingSplitUntil = std::min(c.kingSplitUntil, (int)P_UNK_KING_UNTIL);
    if (c.feedStart >= 300) {
        if (P_E2_FEED > 0) c.feedStart = std::min(c.feedStart, (int)P_E2_FEED);
        if (P_E2_PULL > 0) c.feedPullStart = std::min(c.feedPullStart, (int)P_E2_PULL);
        if (P_E2_GROW > 0) c.growRound = std::min(c.growRound, (int)P_E2_GROW);
    }
    return c;
}
Cfg cfg = CFG_DEFAULT;   // active strategy (set once the map is recognised)
// how long a report of the king stays valid (shorter in the endgame, when kings are fed and replaced)
inline int rvMode() { return cfg.rv > 0 ? cfg.rv : P_RV; }   // v6: per-map meeting tile mode
bool onPortalsMap();
// v14: the round from which every dragon feeds (down to 2 units, margin P_E7_DUMP_MARGIN); P_PORTALS_DUMP on Portals
int mapDump();
inline int dumpRound() { if (P_PORTALS_DUMP > 0 && onPortalsMap()) return (int)P_PORTALS_DUMP; int d = mapDump(); return d > 0 ? d : (int)P_E7_DUMP; }
inline int feedMargin() { return (dumpRound() > 0 && rnd >= dumpRound()) ? P_E7_DUMP_MARGIN : P_FEED_MARGIN; }   // v7: in the last rounds even a near-equal dragon feeds the king
inline int kingFreshN() { return (P_E2_KING_FRESH > 0 && rnd >= cfg.feedPullStart) ? P_E2_KING_FRESH : P_KING_FRESH; }
// known maps where portals lead straight into the enemy's half: keep v3's caution there
const char* const PORTAL_CAUTIOUS[] = { P_PORTAL_CAUTIOUS_MAPS };
int dbMap_();   // (defined below)
bool portalCautious() {
    int m = dbMap_();
    if (m < 0) return false;
    for (const char* n : PORTAL_CAUTIOUS) if (n && !strcmp(n, MAPDB[m].name)) return true;
    return false;
}
// v11: known maps where the opening rush pays (Trophy: the cup; on Devil the rusher lost the centre fight)
const char* const OPEN_RUSH_MAPS[] = { P_OPEN_RUSH_MAPS };
bool openRushMap() {
    int m = dbMap_();
    if (m < 0) return false;
    for (const char* n : OPEN_RUSH_MAPS) if (n && (!strcmp(n, "*") || !strcmp(n, MAPDB[m].name))) return true;
    return false;
}
// v12: known maps where endgame feeding goes into a dragon other than the king only if it is P_LOCALMIN_LEN+ long
const char* const LOCALMIN_MAPS[] = { P_LOCALMIN_MAPS };
bool localMinMap() {
    static int memo = -2, memoFor = -3;
    int m = dbMap_();
    if (memoFor != m) {
        memoFor = m; memo = 0;
        if (m >= 0) for (const char* n : LOCALMIN_MAPS) if (n && !strcmp(n, MAPDB[m].name)) memo = 1;
    }
    return memo == 1;
}
inline int localMinTarget() { return localMinMap() ? std::max((int)P_E2_LOCALMIN, (int)P_LOCALMIN_LEN) : (int)P_E2_LOCALMIN; }
const char* const ZONE_CENTRE_MAPS[] = { P_ZONE_CENTER_MAPS };
bool zoneCentreMap() {
    int m = dbMap_();
    if (m < 0) return false;
    for (const char* n : ZONE_CENTRE_MAPS) if (n && !strcmp(n, MAPDB[m].name)) return true;
    return false;
}
// v13: value factor for pearls an enemy head reaches first; P_COMP_ENEMY_MAPV on the maps listed in P_COMP_ENEMY_MAPS
const char* const COMP_ENEMY_MAPS[] = { P_COMP_ENEMY_MAPS };
double compEnemy() {
    static int memo = -2, memoFor = -3;
    int m = dbMap_();
    if (memoFor != m) {
        memoFor = m; memo = 0;
        if (m >= 0) for (const char* n : COMP_ENEMY_MAPS) if (n && !strcmp(n, MAPDB[m].name)) memo = 1;
    }
    return memo == 1 ? (double)P_COMP_ENEMY_MAPV : (double)P_COMP_ENEMY;
}
// v14: per-map dump round (P_SCH_DUMP, P_SLI_DUMP, P_DEFAULT_DUMP; 0 = P_E7_DUMP)
int mapDump() {
    static int memo = -2, memoFor = -3;
    int m = dbMap_();
    if (memoFor != m) {
        memoFor = m; memo = 0;
        if (m >= 0) {
            const char* n = MAPDB[m].name;
            if (!strcmp(n, "Schooltime")) memo = P_SCH_DUMP;
            else if (!strcmp(n, "Slithery Fight")) memo = P_SLI_DUMP;
            else if (!strcmp(n, "Default")) memo = P_DEFAULT_DUMP;
        }
    }
    return memo;
}
bool onPortalsMap() {
    static int memo = -2, memoFor = -3;
    int m = dbMap_();
    if (memoFor != m) { memoFor = m; memo = (m >= 0 && !strcmp(MAPDB[m].name, "Portals")) ? 1 : 0; }
    return memo == 1;
}
// v14: maps where our dragons keep apart from each other (P_SPREAD_W)
const char* const SPREAD_MAPS[] = { P_SPREAD_MAPS };
bool spreadOn() {
    static int memo = -2, memoFor = -3;
    int m = dbMap_();
    if (memoFor != m) {
        memoFor = m; memo = 0;
        if (m >= 0) for (const char* n : SPREAD_MAPS) if (n && (!strcmp(n, "*") || !strcmp(n, MAPDB[m].name))) memo = 1;
        if (m < 0 && (P_SPREAD_UNKNOWN == 1 || (P_SPREAD_UNKNOWN == 2 && N >= P_BIG_MAP_TILES))) memo = 1;   // v17: unknown maps too (2: big ones only)
    }
    return memo == 1;
}
inline double portalBlind() {
    if (P_PORTALS_BLIND >= 0 && onPortalsMap()) return P_PORTALS_BLIND;
    return portalCautious() ? P_PORTAL_BLIND_CAUTIOUS : P_PORTAL_BLIND;
}
inline double portalExitStay() {
    if (P_PORTALS_EXIT_STAY >= 0 && onPortalsMap()) return P_PORTALS_EXIT_STAY;
    return portalCautious() ? P_PORTAL_EXIT_STAY_CAUTIOUS : P_PORTAL_EXIT_STAY;
}

inline int dbCls(int m, int t) { return (MAPDB[m].tiles[t] - 48) & 7; }
inline bool dbKelpN(int m, int t) { return (MAPDB[m].tiles[t] - 48) >> 3 & 1; }
inline bool dbKelpW(int m, int t) { return (MAPDB[m].tiles[t] - 48) >> 4 & 1; }
bool dbPortal(int m, int isV, int t) {
    const MapDef& M = MAPDB[m];
    for (int i = 0; i < M.nport; i++) if (M.ports[3 * i + 1] == isV && M.ports[3 * i + 2] == t) return true;
    return false;
}
// v14: pearl tiles the known map does not have (the ladder's Queen Of Spades has two small extra patches
// next to the starting dragons, so v7-v13 played it as an unknown layout from round 0). Up to
// P_DB_EXTRA_MAX such tiles keep the map recognised; each gets the gaps P_DB_EXTRA_A..max(P_DB_EXTRA_B, countdown).
std::vector<int> dbExtraT;      // extra pearl tiles found so far (and their mirrors)
int dbExtraFor = -1;            // the map they belong to
std::vector<int> dbExtraNew;    // filled by dbConsistent(m) for the map it checks
inline bool dbIsExtra(int m, int t) {
    if (dbExtraFor != m) return false;
    for (int u : dbExtraT) if (u == t) return true;
    return false;
}
// is the current 7x7 observation consistent with map m?
bool dbConsistent(int m, bool wallsOnly = false) {
    const MapDef& M = MAPDB[m];
    if (M.W != W || M.H != H) return false;
    if (!wallsOnly) dbExtraNew.clear();
    for (int k = 0; k < 49 && !wallsOnly; k++) {
        int t = ID(vtile[k].x, vtile[k].y);
        int c = dbCls(m, t);
        int cd = vtile[k].cd;
        if (c == 7 && cd >= 0 && P_DB_EXTRA_MAX > 0) {
            if (!dbIsExtra(m, t)) {
                dbExtraNew.push_back(t);
                if ((dbExtraFor == m ? (int)dbExtraT.size() : 0) + 2 * (int)dbExtraNew.size() > P_DB_EXTRA_MAX) return false;
            }
            continue;
        }
        if ((c == 7) != (cd < 0)) return false;
        if (c != 7 && cd > M.clsB[c]) return false;
    }
    int hx0 = wx(vtile[0].x + 3), hy0 = wy(vtile[0].y + 3);
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 7; c++) {
            int t = ID(hx0 - 3 + c, hy0 - 3 + r);
            int16_t o = hrow[r][c];
            bool kel = dbKelpN(m, t), por = dbPortal(m, 0, t);
            if ((o == E_KELP) != kel || (o >= 0) != por) return false;
        }
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 8; c++) {
            int t = ID(hx0 - 3 + c, hy0 - 3 + r);
            int16_t o = vrow[r][c];
            bool kel = dbKelpW(m, t), por = dbPortal(m, 1, t);
            if ((o == E_KELP) != kel || (o >= 0) != por) return false;
        }
    return true;
}

TileArr<std::array<int32_t, 4>> nb;    // neighbour table: -1 blocked (kelp or unknown portal exit)
TileArr<std::array<bool, 4>> viaPortal;

int edgeCode(int t, int d, int16_t& e) {
    int x = TX(t), y = TY(t);
    switch (d) {
        case 0: e = eH[t]; return t;
        case 2: { int u = ID(x, y + 1); e = eH[u]; return u; }
        case 3: e = eV[t]; return MAXN + t;
        default: { int u = ID(x + 1, y); e = eV[u]; return MAXN + u; }
    }
}
int stepTrue(int t, int d, bool& portal) {
    int16_t e;
    int code = edgeCode(t, d, e);
    portal = false;
    if (e == E_KELP) return -1;
    if (e >= 0) {
        portal = true;
        int pc = portalPartner(e, code);
        if (pc < 0) return -1;
        int u = pc % MAXN;
        int ux = TX(u), uy = TY(u);
        switch (d) {
            case 0: return ID(ux, uy - 1);
            case 2: return u;
            case 3: return ID(ux - 1, uy);
            default: return u;
        }
    }
    return ID(TX(t) + DX[d], TY(t) + DY[d]);
}
// v8 one-way boxes (known maps): a 2x2 box whose only way in is a portal edge is entered from one side
// of the paired open edge only; the unit walks round the box and comes out on the other side. Units
// entering from both sides met head-on at the exits (Portals: 79 friendly head-ons in one local game).
struct OneWay { std::vector<uint8_t> mask; std::vector<uint8_t> landing; std::vector<uint8_t> enclosed; int n = 0; };
std::map<int, OneWay> oneWayCache;
const OneWay* oneWayCur = nullptr;
int oneWayKey() { return dbMap >= 0 ? dbMap : dbWallsOf; }
void ensureOneWay() {
    oneWayCur = nullptr;
    if (!P_BOX_ONEWAY && !P_ATTEMPT_ENCLOSED) return;
    int key = oneWayKey();
    if (key < 0) return;
    auto it = oneWayCache.find(key);
    if (it != oneWayCache.end()) { oneWayCur = &it->second; return; }
    OneWay& O = oneWayCache[key];
    O.mask.assign(N, 0); O.landing.assign(N, 0);
    std::vector<int> stamp(N, -1), q;
    auto comp = [&](int v, int tag, int cap) {   // tiles reachable from v without portals (stops above cap)
        q.clear(); q.push_back(v); stamp[v] = tag;
        for (size_t i = 0; i < q.size() && (int)q.size() <= cap; i++)
            for (int d = 0; d < 4; d++) {
                bool pt; int w = stepTrue(q[i], d, pt);
                if (w < 0 || pt || stamp[w] == tag) continue;
                stamp[w] = tag; q.push_back(w);
            }
        return (int)q.size();
    };
    int tag = 0;
    // tiles that can only be reached through a portal (a small walled room: the pearl boxes of Portals)
    O.enclosed.assign(N, 0);
    if (P_ATTEMPT_ENCLOSED > 0)
        for (int t = 0; t < N; t++) {
            if (O.enclosed[t]) continue;
            ++tag;
            if (comp(t, tag, P_ATTEMPT_ENCLOSED) <= P_ATTEMPT_ENCLOSED) for (int k : q) O.enclosed[k] = 1;
        }
    for (int t = 0; t < N && P_BOX_ONEWAY; t++)
        for (int d = 0; d < 4; d++) {
            bool pt; int v = stepTrue(t, d, pt);
            if (v < 0 || !pt) continue;
            int t2 = ID(TX(t) + DX[d], TY(t) + DY[d]);
            if (t2 < t) continue;   // each open pair once, from its smaller tile
            bool pt2; int v2 = stepTrue(t2, (d + 2) % 4, pt2);
            if (v2 < 0 || !pt2) continue;
            ++tag;
            if (comp(v, tag, 5) != 4) continue;   // a box of exactly 4 tiles...
            if (stamp[v2] != tag) continue;       // ...that both sides of the open pair lead into
            bool inBox = false;
            for (int k : q) if (k == t || k == t2) inBox = true;
            if (inBox) continue;
            ++tag;
            if (comp(t, tag, 8) <= 6) continue;   // t is in the open, not in another box
            O.mask[t2] |= (uint8_t)(1 << ((d + 2) % 4));   // enter from t only; the exits land on t2
            O.landing[t2] = 1;
            O.n++;
        }
    oneWayCur = &O;
#ifdef DEBUGLOG
    if (getenv("DB_DEBUG")) fprintf(stderr, "ONEWAY map %d: %d boxes\n", key, O.n);
#endif
}
inline bool isLanding(int t) { return oneWayCur && oneWayCur->landing[t]; }
inline bool isEnclosed(int t) { return oneWayCur && !oneWayCur->enclosed.empty() && oneWayCur->enclosed[t]; }
int stepRaw(int t, int d, bool& portal) {
    int v = stepTrue(t, d, portal);
    if (P_BOX_ONEWAY && portal && oneWayCur && (oneWayCur->mask[t] >> d & 1)) { portal = false; return -1; }
    return v;
}
void buildNeighbours() {
    if (P_BOX_ONEWAY || P_ATTEMPT_ENCLOSED) {
        const OneWay* before = oneWayCur;
        ensureOneWay();
        if (P_BOX_ONEWAY && oneWayCur != before) nbFull = true;
    }
    if (nbFull) {
        // tiles whose four edges are all open or unknown just get their torus neighbours (cheap:
        // on a big unknown map this is almost every tile); the others go through stepRaw
        for (int y = 0; y < H; y++) {
            int yu = (y == 0 ? H - 1 : y - 1) * W, yd = (y == H - 1 ? 0 : y + 1) * W, yc = y * W;
            for (int x = 0; x < W; x++) {
                int t = yc + x;
                int xe = x == W - 1 ? 0 : x + 1, xw = x == 0 ? W - 1 : x - 1;
                int16_t en = eH[t], es = eH[yd + x], ew = eV[t], ee = eV[yc + xe];
                auto plainE = [](int16_t e) { return e == E_UNK || e == E_OPEN; };
                if (!(plainE(en) && plainE(es) && plainE(ew) && plainE(ee))) {
                    for (int d = 0; d < 4; d++) nb[t][d] = stepRaw(t, d, viaPortal[t][d]);
                    continue;
                }
                nb[t][0] = yu + x; nb[t][1] = yc + xe; nb[t][2] = yd + x; nb[t][3] = yc + xw;
                viaPortal[t][0] = viaPortal[t][1] = viaPortal[t][2] = viaPortal[t][3] = false;
            }
        }
        nbFull = false;
    } else {
        for (int t : nbDirty)
            for (int d = 0; d < 4; d++) nb[t][d] = stepRaw(t, d, viaPortal[t][d]);
    }
    nbDirty.clear();
}

// ------------------------------------------------------------------ per turn state
int headT, hx, hy;
TileArr<bool> visible;
TileArr<int16_t> freeOther;   // earliest my-move index at which others' segments allow entry
TileArr<int16_t> freeGrow;    // v11 (tail-grow mode 3): the same, assuming dragons with a pearl next to their head eat it
TileArr<int16_t> freeMine;
TileArr<int16_t> occId;       // dragon id on tile (visible) or -1
TileArr<bool> occHead;
TileArr<bool> pearlNow;
std::vector<int> myBody;   // head first
std::vector<int> lastPath;
std::vector<int> lastDirs;  // directions of last move
int lastHead = -1;          // head tile before last move

struct Other {
    int id; bool enemy;
    int head = -1, neck = -1, facing = 0;
    int vis = 0, lenKnown = -1;
    std::vector<std::pair<int, int>> tops;  // headless chain tops (tile, facing toward head)
    int lenEst() const { return lenKnown > 0 ? lenKnown : vis; }
};
std::vector<Other> others;
// body layout sent by a parent to its freshly split child over sonar: (start index, directions)
std::vector<std::pair<int, std::vector<int>>> bodyChunks;
std::vector<std::pair<int, uint64_t>> bodyMsgs;   // (sonar direction, message) to send this turn

void adoptSymmetry(int which) {
    if (symKnown >= 0) return;
    symKnown = which;
    for (int k = 0; k < 3; k++) symAlive[k] = (k == which);
    for (int t = 0; t < N; t++) {
        int q = mirrorT(t, symKnown);
        if (tk[t].dR >= 0 && tk[q].cdR < tk[t].dR) { noteAttempt(tk[q], rnd, true); tk[q].cdR = tk[t].dR; tk[q].cdV = tk[t].dV; }
        if (eHd[t] && eH[t] < 0) { int m = mirrorHE(t, symKnown); if (!eHd[m]) setEH(m, eH[t]); }
        if (eVd[t] && eV[t] < 0) { int m = mirrorVE(t, symKnown); if (!eVd[m]) setEV(m, eV[t]); }
        if (tk[t].gN > 0 && tk[q].gN == 0) {
            tk[q].gN = tk[t].gN; tk[q].gSum = tk[t].gSum; tk[q].gMin = tk[t].gMin; tk[q].gMax = tk[t].gMax;
        }
    }
}

// v14: the gaps of the extra pearl tiles (see dbConsistent)
void applyExtraGaps() {
    if (dbMap < 0 || dbExtraFor != dbMap || gapFor != dbMap) return;
    for (int t : dbExtraT) {
        int b = std::max<int>((int)P_DB_EXTRA_B, tk[t].cdMax);
        gapA[t] = (int16_t)P_DB_EXTRA_A; gapB[t] = (int16_t)std::max<int>(b, (int)P_DB_EXTRA_A);
    }
}
// v14: remember the extra tiles dbConsistent(m) just found, with their mirrors
void noteExtras(int m) {
    if (dbExtraNew.empty()) return;
    if (dbExtraFor != m) { dbExtraT.clear(); dbExtraFor = m; }
    const MapDef& M = MAPDB[m];
    for (int t : dbExtraNew) {
        if (!dbIsExtra(m, t)) dbExtraT.push_back(t);
        if (M.sym >= 0) { int q = mirrorT(t, M.sym); if (dbCls(m, q) == 7 && !dbIsExtra(m, q)) dbExtraT.push_back(q); }
    }
    dbExtraNew.clear();
    applyExtraGaps();
}
void applyDB(int m) {
    dbMap = m;
    cfg = cfgFor(m);
    const MapDef& M = MAPDB[m];
    for (int t = 0; t < N; t++) {
        eH[t] = dbKelpN(m, t) ? E_KELP : E_OPEN;
        eV[t] = dbKelpW(m, t) ? E_KELP : E_OPEN;
        int c = dbCls(m, t);
        if (c == 7) { gapA[t] = 0; gapB[t] = 0; } else { gapA[t] = (int16_t)M.clsA[c]; gapB[t] = (int16_t)M.clsB[c]; }
    }
    gapFor = m;
    applyExtraGaps();
    portals.clear();
    for (int i = 0; i < M.nport; i++) {
        int pid = M.ports[3 * i], isV = M.ports[3 * i + 1], t = M.ports[3 * i + 2];
        if (isV) eV[t] = (int16_t)pid; else eH[t] = (int16_t)pid;
        portals.push_back({pid, isV * MAXN + t});
    }
    nbFull = true; nbDirty.clear();
    if (M.sym >= 0 && symKnown < 0) adoptSymmetry(M.sym);
}
// v7: the ladder plays variants of the known maps (same walls, other pearl tiles). Keep the walls,
// portals, symmetry and strategy row of the map; learn the pearls as on an unknown map.
void dropDBKeepWalls(int m) {
    dbMap = -1; dbWallsOf = m;
    cfg = cfgFor(m);
    nbFull = true; nbDirty.clear();
}
void dropDB() {
    dbMap = -1; dbWallsOf = -1;
    cfg = cfgFor(-1);
    portals.clear();
    for (int t = 0; t < N; t++) {
        if (!eHd[t]) eH[t] = E_UNK; else if (eH[t] >= 0) portals.push_back({eH[t], t});
        if (!eVd[t]) eV[t] = E_UNK; else if (eV[t] >= 0) portals.push_back({eV[t], MAXN + t});
    }
    symKnown = -1; symAlive[0] = symAlive[1] = symAlive[2] = true;
    nbFull = true; nbDirty.clear();
}
uint32_t dbCand = 0xFFFFFFFFu;   // candidate maps still consistent with everything seen
uint32_t dbCandW = 0xFFFFFFFFu;  // v7: candidate maps whose walls and portals fit everything seen

void calibrate();
void updateKnowledge() {
    hx = wx(vtile[0].x + 3); hy = wy(vtile[0].y + 3);
    headT = ID(hx, hy);
    if (P_MAPDB) {
        if (!dbTried) {
            int cnt = 0, found = -1, first = -1, cntW = 0, foundW = -1;
            std::vector<int> foundX, firstX;   // v14: extra pearl tiles seen, per fitting map
            for (int m = 0; m < NMAPDB && m < 32; m++) {
                if (P_DB_VARIANT && (dbCandW >> m & 1)) {
                    if (dbConsistent(m, true)) { cntW++; foundW = m; } else dbCandW &= ~(1u << m);
                }
                if (!(dbCand >> m & 1)) continue;
                if (dbConsistent(m)) { cnt++; found = m; foundX = dbExtraNew; if (first < 0) { first = m; firstX = dbExtraNew; } } else dbCand &= ~(1u << m);
            }
            if (cnt == 1) {
                if (dbMap != found) { if (dbMap >= 0) dropDB(); applyDB(found); }
                dbExtraNew = foundX; noteExtras(found);
                if (!P_DB_SPAWNS) dropDBKeepWalls(found);   // v7.1 option: never trust the stored pearl layout
                dbTried = true;
#ifdef DEBUGLOG
                if (getenv("DB_DEBUG")) fprintf(stderr, "r%d id%d DB=%s\n", rnd, myId, MAPDB[found].name);
#endif
            }
            else if (cnt == 0 && P_DB_VARIANT && cntW >= 1) {
                if (dbMap >= 0) dropDB();
                if (cntW == 1) {   // a variant of exactly one known map
                    applyDB(foundW); dropDBKeepWalls(foundW); dbTried = true;
#ifdef DEBUGLOG
                    if (getenv("DB_DEBUG")) fprintf(stderr, "r%d id%d DB variant of %s\n", rnd, myId, MAPDB[foundW].name);
#endif
                }
            }
            else if (cnt == 0) { if (dbMap >= 0) dropDB(); dbTried = true; }
            else if (P_DB_TENTATIVE && (dbMap < 0 || !(dbCand >> dbMap & 1))) {
                // several known maps fit what I have seen (Stronghold/Trauma look alike in places):
                // use one of them for now; everything in view agrees, and a contradiction switches maps
                if (dbMap >= 0) dropDB();
                applyDB(first);
                dbExtraNew = firstX; noteExtras(first);
                if (!P_DB_SPAWNS) dropDBKeepWalls(first);
#ifdef DEBUGLOG
                if (getenv("DB_DEBUG")) fprintf(stderr, "r%d id%d DB tentative %s (%d fit)\n", rnd, myId, MAPDB[first].name, cnt);
#endif
            }
        } else if (dbMap >= 0 && !dbConsistent(dbMap)) {
#ifdef DEBUGLOG
            if (getenv("DB_DEBUG")) fprintf(stderr, "r%d id%d DB DROPPED%s\n", rnd, myId, P_DB_VARIANT && dbConsistent(dbMap, true) ? " (walls kept: variant)" : "");
#endif
            if (P_DB_VARIANT && dbConsistent(dbMap, true)) dropDBKeepWalls(dbMap);
            else dropDB();
        } else if (dbWallsOf >= 0 && !dbConsistent(dbWallsOf, true)) {
            dropDB();
        } else if (dbMap >= 0) {
            noteExtras(dbMap);   // v14: dbConsistent(dbMap) above passed; keep the extra pearl tiles it saw
        }
    }
#ifdef PORTAL_LOG
    if (turnCount < 3) {
        char b[96];
        for (int r = 0; r < 8; r++) for (int c = 0; c < 7; c++) if (hrow[r][c] >= 0) { int t = ID(hx - 3 + c, hy - 3 + r); snprintf(b, sizeof b, "LOG PH %d %d %d", TX(t), TY(t), hrow[r][c]); emit(b); }
        for (int r = 0; r < 7; r++) for (int c = 0; c < 8; c++) if (vrow[r][c] >= 0) { int t = ID(hx - 3 + c, hy - 3 + r); snprintf(b, sizeof b, "LOG PV %d %d %d", TX(t), TY(t), vrow[r][c]); emit(b); }
    }
#endif
    for (int t = 0; t < N; t++) visible[t] = false;
    if (P_CALIB) calibrate();
    for (int k = 0; k < 49; k++) {
        int t = ID(vtile[k].x, vtile[k].y);
        visible[t] = true;
        TileK& K = tk[t];
        int cd = vtile[k].cd;
        if (cd >= 0 && K.cdR >= 0 && K.cdV >= 0 && K.cdR + K.cdV == rnd) {
            K.gN++; K.gSum += cd; K.gMin = std::min<int>(K.gMin, cd); K.gMax = std::max<int>(K.gMax, cd);
        }
        noteAttempt(K, rnd); K.cdR = rnd; K.cdV = cd; K.dR = rnd; K.dV = cd;
        if (cd > K.cdMax) K.cdMax = (int16_t)cd;
        K.pR = rnd; K.pV = vtile[k].pearl != 0;
    }
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 7; c++) {
            int t = ID(hx - 3 + c, hy - 3 + r);
            eHd[t] = true;
            if (dbMap >= 0) continue;
            setEH(t, hrow[r][c]);
            if (hrow[r][c] >= 0) addPortal(hrow[r][c], t);
        }
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 8; c++) {
            int t = ID(hx - 3 + c, hy - 3 + r);
            eVd[t] = true;
            if (dbMap >= 0) continue;
            setEV(t, vrow[r][c]);
            if (vrow[r][c] >= 0) addPortal(vrow[r][c], MAXN + t);
        }
    if (symKnown < 0) {
        for (int k = 0; k < 3; k++) {
            if (!symAlive[k]) continue;
            bool bad = false;
            for (int i = 0; i < 49 && !bad; i++) {
                int t = ID(vtile[i].x, vtile[i].y);
                int q = mirrorT(t, k);
                if (q == t || tk[q].dR < 0) continue;
                int c1 = vtile[i].cd, c2 = tk[q].dV, r2 = tk[q].dR;
                if ((c1 < 0) != (c2 < 0)) { bad = true; break; }
                if (c1 >= 0 && r2 + c2 > rnd && rnd + c1 != r2 + c2) bad = true;
            }
            for (int r = 0; r < 8 && !bad; r++)
                for (int c = 0; c < 7 && !bad; c++) {
                    int t = ID(hx - 3 + c, hy - 3 + r);
                    int q = mirrorHE(t, k);
                    if (eHd[q] && kindOf(eH[q]) != kindOf(eH[t])) bad = true;
                }
            for (int r = 0; r < 7 && !bad; r++)
                for (int c = 0; c < 8 && !bad; c++) {
                    int t = ID(hx - 3 + c, hy - 3 + r);
                    int q = mirrorVE(t, k);
                    if (eVd[q] && kindOf(eV[q]) != kindOf(eV[t])) bad = true;
                }
            if (bad) symAlive[k] = false;
        }
        int alive = 0, which = -1;
        for (int k = 0; k < 3; k++) if (symAlive[k]) { alive++; which = k; }
        if (alive == 1) adoptSymmetry(which);
    }
    if (symKnown >= 0) {
        for (int i = 0; i < 49; i++) {
            int t = ID(vtile[i].x, vtile[i].y);
            int q = mirrorT(t, symKnown);
            if (!visible[q]) {
                noteAttempt(tk[q], rnd, true); tk[q].cdR = rnd; tk[q].cdV = vtile[i].cd;
                if (vtile[i].cd > tk[q].cdMax) tk[q].cdMax = (int16_t)vtile[i].cd;
                if (tk[t].gN > tk[q].gN) {
                    tk[q].gN = tk[t].gN; tk[q].gSum = tk[t].gSum; tk[q].gMin = tk[t].gMin; tk[q].gMax = tk[t].gMax;
                }
            }
        }
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 7; c++) {
                int t = ID(hx - 3 + c, hy - 3 + r);
                int m = mirrorHE(t, symKnown);
                if (!eHd[m] && eH[t] < 0) setEH(m, eH[t]);
            }
        for (int r = 0; r < 7; r++)
            for (int c = 0; c < 8; c++) {
                int t = ID(hx - 3 + c, hy - 3 + r);
                int m = mirrorVE(t, symKnown);
                if (!eVd[m] && eV[t] < 0) setEV(m, eV[t]);
            }
    }
    // learn a portal's far end by having walked through it
    if (lastDirs.size() == 1 && lastHead >= 0) {
        int d = lastDirs[0];
        int16_t e;
        int code = edgeCode(lastHead, d, e);
        if (e >= 0 && portalPartner(e, code) < 0) {
            int u = headT, pc;
            switch (d) {
                case 0: pc = ID(TX(u), TY(u) + 1); break;
                case 2: pc = u; break;
                case 3: pc = MAXN + ID(TX(u) + 1, TY(u)); break;
                default: pc = MAXN + u; break;
            }
            if (pc != code) {
                addPortal(e, pc);
                lastPath.assign(1, headT);
            }
        }
    }
    // mirrored portal pairs
    if (symKnown >= 0) {
        auto mcode = [&](int c) { return c < MAXN ? mirrorHE(c, symKnown) : MAXN + mirrorVE(c - MAXN, symKnown); };
        size_t np = portals.size();
        for (size_t i = 0; i < np; i++) {
            int pa = portals[i].code;
            int pb = portalPartner(portals[i].id, pa);
            if (pb < 0) continue;
            int ma = mcode(pa), mb = mcode(pb);
            int ia = -1, ib = -1;
            for (size_t j = 0; j < portals.size(); j++) {
                if (portals[j].code == ma) ia = j;
                if (portals[j].code == mb) ib = j;
            }
            if (ia >= 0 && ib < 0) addPortal(portals[ia].id, mb);
            else if (ib >= 0 && ia < 0) addPortal(portals[ib].id, ma);
        }
    }
}

void buildDragons() {
    for (int t = 0; t < N; t++) {
        occId[t] = -1; occHead[t] = false; freeOther[t] = 0; freeMine[t] = 0; pearlNow[t] = false; freeGrow[t] = 0;
    }
    for (int k = 0; k < 49; k++) if (vtile[k].pearl) pearlNow[ID(vtile[k].x, vtile[k].y)] = true;
    static TileArr<int16_t> partIdx;
    static TileArr<int> pred, nxtOf;
    for (auto& p : vparts) { int t = ID(p.x, p.y); partIdx[t] = -1; pred[t] = -1; nxtOf[t] = -1; }
    for (int i = 0; i < (int)vparts.size(); i++) {
        int t = ID(vparts[i].x, vparts[i].y);
        partIdx[t] = i; occId[t] = vparts[i].id; occHead[t] = vparts[i].head;
    }
    for (int i = 0; i < (int)vparts.size(); i++) {
        const VPart& p = vparts[i];
        int t = ID(p.x, p.y);
        if (p.head) continue;
        int n = nb[t][p.f];
        if (n < 0) n = ID(p.x + DX[p.f], p.y + DY[p.f]);
        if (occId[n] == p.id && partIdx[n] >= 0 && pred[n] == -1) { pred[n] = t; nxtOf[t] = n; }
    }
    // my last move went through a portal I did not know the far end of: now that it is known
    // (walked through, or its exit edge came into view), rebuild the path so my body history
    // continues through the portal (otherwise the neck behind the portal is invisible and
    // stepping back through the portal kills me)
    if (P_PORTAL_PATHFIX && !lastDirs.empty() && lastHead >= 0 && lastPath.size() < lastDirs.size()) {
        std::vector<int> pth;
        int cur = lastHead;
        for (int d : lastDirs) { int v = nb[cur][d]; if (v < 0) break; pth.push_back(v); cur = v; }
        if (pth.size() == lastDirs.size() && pth.back() == headT) lastPath = pth;
    }
    others.clear();
    std::vector<int> ids;
    for (auto& p : vparts) if (p.id != myId && std::find(ids.begin(), ids.end(), p.id) == ids.end()) ids.push_back(p.id);
    for (int id : ids) {
        Other D; D.id = id;
        for (auto& p : vparts) if (p.id == id) {
            D.enemy = p.team != myTeam; D.vis++;
            if (p.head) { D.head = ID(p.x, p.y); D.facing = p.f; }
        }
        for (auto& p : vparts) {
            if (p.id != id) continue;
            int t = ID(p.x, p.y);
            if (!p.head && nxtOf[t] >= 0) continue;  // not a chain top
            if (!p.head) D.tops.push_back({t, p.f});
            std::vector<int> chain;
            int cur = t;
            while (cur >= 0 && (int)chain.size() < 5000) { chain.push_back(cur); cur = pred[cur]; }
            int end = chain.back();
            bool certain = true;
            for (int d = 0; d < 4 && certain; d++) {
                int q = nb[end][d];
                if (q < 0) { if (viaPortal[end][d]) certain = false; continue; }
                if (!visible[q]) certain = false;
            }
            int n = chain.size();
            // v10: a dragon that eats (a pearl next to its head) or splits on its next move leaves its tail
            // where it is one turn longer: on the ladder our units died walking into an ally's tail that
            // stayed (Schooltime: about 90 "doomed" deaths a game from growth and splits)
            int extra = 0;
            const int tgm = tailGrowMode();
            if (p.head && (tgm || P_TAIL_SPLIT)) {
                bool grow = false;
                if (tgm && (tgm != 2 || !D.enemy))   // 2: allies only
                    for (int d = 0; d < 4; d++) {
                        int v = nb[t][d];
                        if (v >= 0 && (n < 2 || v != chain[1]) && visible[v] && pearlNow[v] && occId[v] < 0) grow = true;
                    }
                bool spl = P_TAIL_SPLIT && D.enemy == false && n >= P_TAIL_SPLIT && cfg.splitMin < 999 && unitCount < UNIT_LIMIT;
                if (grow || spl) extra = 1;
            }
            for (int i = 0; i < n; i++) {
                int b = certain ? (n - 1 - i) : BIG_FREE;
                if (tgm == 3) {   // v11: only the survival search assumes the eater's tail stays
                    freeOther[chain[i]] = (int16_t)std::min(BIG_FREE, b + 2);
                    freeGrow[chain[i]] = (int16_t)std::min(BIG_FREE, b + 2 + extra);
                } else
                    freeOther[chain[i]] = (int16_t)std::min(BIG_FREE, b + 2 + extra);
            }
            if (!p.head && P_PORTAL_TOP_BLOCK) {
                // v7.1: this body goes on through a portal, so its head part is on the other side, starting at
                // the exit tile: exactly where a dragon following it through would land (on the ladder 10-25%
                // of our portal crossings ended in a head-on with an ally there)
                int x = nb[t][p.f];
                if (x >= 0 && viaPortal[t][p.f] && !visible[x]) {
                    int b = certain ? n + 1 : BIG_FREE;   // it frees once the rest of the body has passed
                    freeOther[x] = (int16_t)std::max<int>(freeOther[x], std::min(BIG_FREE, b + 1));
                }
            }
            if (p.head) {
                if (n >= 2) D.neck = chain[1];
                if (certain) D.lenKnown = n;
            }
        }
        others.push_back(D);
    }
    // my body: the visible chain from the head, extended by history when that is consistent
    // (a freshly split child may not see its whole body; history fills it in as it moves)
    std::vector<int> vis;
    {
        int cur = headT;
        while (cur >= 0 && (int)vis.size() < myLen) { vis.push_back(cur); cur = pred[cur]; }
    }
    bool ok = !myBody.empty();
    std::vector<int> nbd;
    if (ok) {
        for (int i = (int)lastPath.size() - 1; i >= 0; i--) nbd.push_back(lastPath[i]);
        for (int t : myBody) nbd.push_back(t);
        if ((int)nbd.size() > myLen) nbd.resize(myLen);
        if (nbd.empty() || nbd[0] != headT) ok = false;
        for (size_t i = 0; ok && i < nbd.size() && i < vis.size(); i++) if (nbd[i] != vis[i]) ok = false;
        if (ok) for (int t : nbd) if (visible[t] && occId[t] != myId) { ok = false; break; }
        if (ok && vis.size() > nbd.size()) ok = false;
    }
#ifdef DEBUGLOG
    if (getenv("BODYRESET") && !ok && !myBody.empty() && (int)vis.size() < myLen) {
        int why = 0;
        if (nbd.empty() || nbd[0] != headT) why = 1;
        else {
            for (size_t i = 0; i < nbd.size() && i < vis.size(); i++) if (nbd[i] != vis[i]) { why = 2; break; }
            if (!why) for (int t : nbd) if (visible[t] && occId[t] != myId) { why = 3; break; }
            if (!why && vis.size() > nbd.size()) why = 4;
        }
        fprintf(stderr, "BODYRESET r%d id%d len%d vis%zu hist%zu why%d head(%d,%d) nbd0(%d,%d) lp%zu lastHead(%d,%d) body0(%d,%d)\n", rnd, myId, myLen, vis.size(), nbd.size(), why, hx, hy,
                nbd.empty() ? -1 : TX(nbd[0]), nbd.empty() ? -1 : TY(nbd[0]), lastPath.size(), lastHead >= 0 ? TX(lastHead) : -1, lastHead >= 0 ? TY(lastHead) : -1, TX(myBody[0]), TY(myBody[0]));
    }
#endif
    myBody = ok ? nbd : vis;
    if (!ok && dbMap >= 0 && rnd == 0 && turnCount == 0 && (int)vis.size() < myLen) {
        // a starting dragon: the map file tells us the whole body
        const MapDef& M = MAPDB[dbMap];
        int p = 0;
        for (int i = 0; i < M.ndr; i++) {
            int n = M.drg[p + 1];
            if (n == myLen && M.drg[p + 2] == headT) {
                std::vector<int> sb(M.drg + p + 2, M.drg + p + 2 + n);
                bool good = true;
                for (size_t j = 0; good && j < sb.size(); j++) {
                    if (sb[j] < 0 || sb[j] >= N) good = false;
                    else if (visible[sb[j]] && occId[sb[j]] != myId) good = false;
                    else if (j < vis.size() && sb[j] != vis[j]) good = false;
                }
                if (good) { myBody = sb; break; }
            }
            p += 2 + n;
        }
    }
    if (P_START_EXTRAP && !ok && dbMap < 0 && rnd == 0 && turnCount == 0 && firstRound == 0 && vis.size() >= 2 && (int)vis.size() < myLen &&
        (int)myBody.size() < myLen) {
        // a starting dragon on an unknown map whose body leaves the view: assume it goes on straight
        // (starting bodies are laid out in lines); wrong guesses are corrected by next turn's view
        int k = vis.size(), d = -1;
        for (int e = 0; e < 4; e++) if (nb[vis[k - 2]][e] == vis[k - 1]) d = e;
        std::vector<int> ext = vis;
        bool good = d >= 0;
        while (good && (int)ext.size() < myLen) {
            int nx = nb[ext.back()][d];
            if (nx < 0 || visible[nx]) { good = false; break; }
            for (int t : ext) if (t == nx) good = false;
            ext.push_back(nx);
        }
        if (good) myBody = ext;
    }
    if (!ok && !bodyChunks.empty() && (int)vis.size() < myLen) {
        // extend the visible chain with the layout my parent sent me
        std::sort(bodyChunks.begin(), bodyChunks.end());
        std::vector<int> ext = vis;
        for (auto& ch : bodyChunks) {
            if (ch.first >= (int)ext.size()) break;
            if ((ext[ch.first] % 61) != ch.second[0]) continue;   // anchor tile mismatch: not my body
            ext.resize(ch.first + 1);
            for (size_t q = 1; q < ch.second.size(); q++) {
                int d = ch.second[q];
                if ((int)ext.size() >= myLen) break;
                int nx = nb[ext.back()][d];
                if (nx < 0) break;
                ext.push_back(nx);
            }
        }
        bool good = ext.size() > vis.size();
        for (size_t i = 0; good && i < ext.size(); i++) {
            int t = ext[i];
            if (visible[t] && occId[t] != myId) good = false;
            for (size_t j = 0; good && j < i; j++) if (ext[j] == t) good = false;
        }
#ifdef DEBUGLOG
        if (getenv("BODY_DEBUG")) fprintf(stderr, "r%d id%d len%d vis%zu chunks%zu ext%zu good%d\n", rnd, myId, myLen, vis.size(), bodyChunks.size(), ext.size(), (int)good);
#endif
        if (good) myBody = ext;
    }
    bodyChunks.clear();
    int L = myBody.size();
    for (int i = 0; i < L; i++) freeMine[myBody[i]] = (int16_t)(myLen - i + 1);
    // visible parts of my own body that are not in the known chain (a coiled body leaving and
    // re-entering the view): position in the body unknown, so assume the latest possible free time
    if (L < myLen)
        for (auto& p : vparts)
            if (p.id == myId) {
                int t = ID(p.x, p.y);
                if (freeMine[t] == 0) freeOther[t] = (int16_t)std::max<int>(freeOther[t], std::min(BIG_FREE, myLen - L + 2));
            }
    lastPath.clear();
}

// ------------------------------------------------------------------ tile pearl model
double gpow[1024];
double fpow[1024];
TileArr<double> pvNow;
TileArr<int16_t> spawnIn;
TileArr<bool> farm;
TileArr<double> compF;

double gapEst(int t) {
    if (tk[t].gN > 0) return (double)tk[t].gSum / tk[t].gN;
    // a countdown seen at a random moment is on average a third of the tile's maximum gap
    if (P_GAP_CD > 0 && tk[t].cdMax > 0) return std::max(1.0, (double)P_GAP_CD * tk[t].cdMax);
    return P_GAP_PRIOR;
}

double decayTab[2048], decay3Tab[2048];
inline double decayF(int d) { return d < 2048 ? decayTab[d < 0 ? 0 : d] : 0.0; }
inline double decay3F(int d) { return d < 2048 ? decay3Tab[d < 0 ? 0 : d] : 0.0; }

// P(a gap drawn uniformly from [a,b] is <= e)
inline double gapCdf(int a, int b, int e) {
    if (e < a) return 0.0;
    if (e >= b) return 1.0;
    return (double)(e - a + 1) / (b - a + 1);
}

// ---- the pearl estimate of a tile out of view, split by the kind of evidence it rests on.
// With P_CALIB each kind gets a factor learned online: when a tile comes into view, what the
// model predicted for it is compared with what is actually there.
enum { PC_SEEN = 0, PC_ATTEMPT = 1, PC_RESPAWN = 2, PC_UNSEEN_DB = 3, PC_UNSEEN = 4, PC_MEMO = 5, PC_N = 6 };   // PC_MEMO: v7.1 remembered attempts
double calA[PC_N], calB[PC_N], calF[PC_N] = {1, 1, 1, 1, 1, 1};
void updateCalF() {
    for (int c = 0; c < PC_N; c++)
        calF[c] = P_CALIB ? std::max(0.05, std::min((double)P_CALIB_MAX, (calB[c] + P_CALIB_K) / (calA[c] + P_CALIB_K))) : 1.0;
}
TileArr<int16_t> homeAdv;     // (distance from enemy starts) - (distance from our starts); > 0 = our side
int homeFor = -1, homeSide = -1;
// v10: which starting side of a known map is ours (0/1 = the map file's team index). The ladder now
// starts team A on the map file's team-1 tiles in about half the games; v7-v9 took the side from the
// team letter, so the Portals mirror fix, the memory side filter and the roles worked for the wrong side
int sideIdx = -1, spawnHead = -1;
int mySide() { return (P_SIDE_DETECT && sideIdx >= 0) ? sideIdx : (myTeam == 'A' ? 0 : 1); }
void buildHome();
int modelTerms(int t, double* term, int16_t& spawn) {
    const TileK& K = tk[t];
    for (int c = 0; c < PC_N; c++) term[c] = 0;
    spawn = -1;
    if (K.pR >= 0 && K.pV) term[PC_SEEN] = decayF(rnd - K.pR);
    if (dbMap >= 0) {
        int a = gapA[t], b = gapB[t];
        if (b <= 0) return -1;
        if (K.cdR >= 0 && K.cdV >= 0) {
            int S = K.cdR + K.cdV;  // spawn attempt round known as of cdR
            if (S > rnd) {
                spawn = (int16_t)(S - rnd);
                // v7: a countdown copied from the mirrored tile says nothing about whether a pearl is
                // already lying here (countdowns keep running while a pearl waits). Never looked at:
                // earlier attempts have almost surely put one there. Seen empty before the last attempt
                // (S - gap, gap in [a,b]): it may have spawned since.
                if (P_MIRROR_FIX && (P_MIRROR_FIX == 2 || (homeFor == dbMap && homeAdv[t] >= 0))) {   // our side (by path) only: the enemy's half pulled us into fights (Dilemma)
                    // v11: chance that the attempt before S fell in [from, rnd]. v7-v10 used the chance that
                    // it fell anywhere before S, which on slow tiles (Default: gaps up to 3849) is large even
                    // when S is far in the future, so our units chased pearls that were not there. S may
                    // also be the tile's first attempt (the initial countdown), with no attempt before it.
                    auto prevIn = [&](int from) {
                        int lo = std::max(S - b, 1), hi = std::min(S - a, rnd);
                        int nAll = std::max(0, hi - lo + 1), nWin = std::max(0, hi - std::max(lo, from) + 1);
                        double w = 2.0 / (a + b), first = (S >= a && S <= b) ? 1.0 : 0.0;
                        double den = nAll * w + first;
                        return den > 0 ? nWin * w / den : 0.0;
                    };
                    if (K.pR < 0) {
                        double q = P_PREV_FIX ? prevIn(1) : gapCdf(a, b, rnd + 1);
                        double tau = 3.0 * P_DECAY;
                        double el = std::max(1, rnd + 2 - a);
                        double phi = std::min(1.0, tau / el * (1.0 - std::exp(-el / tau)));
                        double surv = P_UNSEEN_SURV + (cfg.unseenEarly > 0 ? (1.0 - P_UNSEEN_SURV) * std::max(0.0, 1.0 - rnd / (double)cfg.unseenEarly) : 0.0);
                        term[PC_UNSEEN_DB] = q * phi * surv;
                    } else if (!K.pV && K.cdR > K.pR && S - a > K.pR) {
                        double q = P_PREV_FIX ? prevIn(K.pR + 1) : gapCdf(a, b, S - K.pR - 1);   // P(previous attempt after I last saw it empty)
                        if (q > 0) term[PC_RESPAWN] = q * P_RESPAWN_SURV * decay3F(std::max(0, rnd - K.pR) / 2);
                    }
                }
            } else {
                if (S > K.pR) term[PC_ATTEMPT] = 0.9 * decayF(rnd - S);
                // later spawns: next attempt at S + U[a,b]
                double q = gapCdf(a, b, rnd - S);
                if (q > 0) term[PC_RESPAWN] = q * P_RESPAWN_SURV * decay3F((rnd - S) / 2);
            }
        } else {
            // never observed: first spawn after the initial countdown U[a,b]
            double q = gapCdf(a, b, rnd + 1);
            double tau = 3.0 * P_DECAY;
            double el = std::max(1, rnd + 2 - a);
            double phi = std::min(1.0, tau / el * (1.0 - std::exp(-el / tau)));
            // early on almost nobody has been anywhere: unseen pearls are very likely still there
            double surv = P_UNSEEN_SURV + (cfg.unseenEarly > 0 ? (1.0 - P_UNSEEN_SURV) * std::max(0.0, 1.0 - rnd / (double)cfg.unseenEarly) : 0.0);
            term[PC_UNSEEN_DB] = q * phi * surv;
        }
    } else {
        if (K.cdR >= 0 && K.cdV < 0) return -1;  // never spawns
        if (K.cdR >= 0) {
            int sr = K.cdR + K.cdV;
            if (sr > rnd) spawn = (int16_t)(sr - rnd);
            else if (sr > K.pR) term[PC_ATTEMPT] = 0.85 * decayF(rnd - sr);
            // v7: countdown known only from the mirrored tile, pearl never looked at: a spawn tile
            // that has had attempts before (see the DB branch)
            if (P_MIRROR_FIX && P_MIRROR_UNSEEN_P > 0 && K.pR < 0 && rnd >= 2) term[PC_UNSEEN_DB] = P_MIRROR_UNSEEN_P;   // own calibration class (unused without the DB)
        } else if (K.pR < 0) {
            term[PC_UNSEEN] = P_UNKNOWN_PRIOR;
        }
        if (K.pR >= 0 && !K.pV && spawn < 0) {
            double g = gapEst(t);
            int since = rnd - std::max<int>(K.pR, K.cdR >= 0 ? K.cdR + K.cdV : K.pR);
            if (since > 0) term[PC_RESPAWN] = std::min(0.8, since / (2.0 * g)) * decay3F(since);
        }
    }
    // v7.1: an attempt since I last looked (or ever, if I never looked) most likely left a pearl there
    // (mirror-derived attempts only on our side of the map: on Trophy the enemy's half lured us away)
    // (only on fast tiles: a remembered pearl of a slow tile is too often gone by the time we get there - Trophy)
    if (P_ATTEMPT_MEMORY && K.aR >= 0 && K.aR > K.pR && K.aR <= rnd && (!K.aMir || (homeFor >= 0 && homeAdv[t] >= P_ATTEMPT_SIDE)) &&
        (dbMap >= 0 ? gapB[t] <= P_ATTEMPT_MAXGAP : (K.cdMax >= 0 && K.cdMax <= P_ATTEMPT_MAXGAP)) &&
        (!P_ATTEMPT_ENCLOSED || isEnclosed(t)))   // v8: only in rooms behind portals (elsewhere it cost games)
        term[P_ATTEMPT_OWN_CLASS ? PC_MEMO : PC_ATTEMPT] = std::max(term[P_ATTEMPT_OWN_CLASS ? PC_MEMO : PC_ATTEMPT], (dbMap >= 0 ? 0.9 : 0.85) * decayF(rnd - K.aR));
    int best = -1; double bv = 0;
    for (int c = 0; c < PC_N; c++) { double v = term[c] * calF[c]; if (v > bv) { bv = v; best = c; } }
    return best;
}

// expected-pearl model for tile t (fills pvNow/spawnIn/farm)
long cntPM = 0, cntSim = 0, cntBFS = 0, cntSurv = 0, cntCut = 0;   // CPU_TRACE counters
void pearlModel(int t) {
    cntPM++;
    const TileK& K = tk[t];
    spawnIn[t] = -1; pvNow[t] = 0; farm[t] = false;
    if (dbMap >= 0) {
        if (gapB[t] <= 0) return;
        if (gapB[t] <= P_FARM_MAXGAP) farm[t] = true;
    } else {
        if (K.cdR >= 0 && K.cdV < 0) return;  // never spawns
        if (K.gN > 0 && K.gMax <= 3) farm[t] = true;
    }
    if (visible[t]) {
        pvNow[t] = pearlNow[t] ? 1.0 : 0.0;
        if (K.cdV >= 0) spawnIn[t] = K.cdV;
        return;
    }
    double term[PC_N]; int16_t sp;
    modelTerms(t, term, sp);
    spawnIn[t] = sp;
    double p = 0;
    for (int c = 0; c < PC_N; c++) p = std::max(p, term[c] * calF[c]);
    pvNow[t] = std::min(1.0, p);
}

// compare the model's prediction for tiles that come into view (before this turn's observation
// overwrites what I knew about them) with what is there
void calibrate() {
    for (int c = 0; c < PC_N; c++) { calA[c] *= P_CALIB_FORGET; calB[c] *= P_CALIB_FORGET; }
    if (turnCount > 0) {
        static TileArr<bool> occ;
        for (auto& p : vparts) occ[ID(p.x, p.y)] = true;
        for (int k = 0; k < 49; k++) {
            int t = ID(vtile[k].x, vtile[k].y);
            if (occ[t] || vtile[k].cd < 0) continue;      // bodies hide pearls; tiles that never spawn
            if (tk[t].pR >= rnd - 1) continue;            // was in view last turn: nothing to learn
            if (dbMap >= 0 && gapB[t] <= 0) continue;
            double term[PC_N]; int16_t sp;
            int c = modelTerms(t, term, sp);
            if (c < 0) continue;
            calA[c] += term[c]; calB[c] += vtile[k].pearl ? 1.0 : 0.0;
        }
        for (auto& p : vparts) occ[ID(p.x, p.y)] = false;
    }
    updateCalF();
#ifdef DEBUGLOG
    if (getenv("CALIB_DEBUG") && rnd % 25 == 0)
        fprintf(stderr, "CAL r%d id%d db%d seen %.2f(%.1f/%.1f) att %.2f(%.1f/%.1f) resp %.2f(%.1f/%.1f) unDB %.2f(%.1f/%.1f) un %.2f(%.1f/%.1f)\n", rnd, myId, dbMap,
                calF[0], calB[0], calA[0], calF[1], calB[1], calA[1], calF[2], calB[2], calA[2], calF[3], calB[3], calA[3], calF[4], calB[4], calA[4]);
#endif
}

// ------------------------------------------------------------------ BFS utilities
struct DistMap {
    TileArr<int16_t> d{1, UNR};
    TileArr<int> list;
    int n = 0;
    DistMap() {}
    void clear() { for (int i = 0; i < n; i++) d[list[i]] = UNR; n = 0; }
    inline void set(int t, int v) { d[t] = (int16_t)v; list[n++] = t; }
};

// time-aware BFS from `start` at time t0; tile v enterable at my move s if its free time <= s.
// `useMine` includes my current body's free times.
void timeBFS(DistMap& M, int start, int t0, int maxD, bool useMine = true) {
    M.clear();
    M.set(start, t0);
    for (int qh = 0; qh < M.n; qh++) {
        int u = M.list[qh];
        int du = M.d[u];
        if (du >= maxD) continue;
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || M.d[v] != UNR) continue;
            int fr = freeOther[v];
            if (useMine && freeMine[v] > fr) fr = freeMine[v];
            if (fr > du + 1) continue;
            M.set(v, du + 1);
        }
    }
}

// BFS from another dragon's head: its move k lands after my k-th move.
void otherBFS(DistMap& M, int start, int maxD) {
    M.clear();
    M.set(start, 0);
    for (int qh = 0; qh < M.n; qh++) {
        int u = M.list[qh];
        int du = M.d[u];
        if (du >= maxD) continue;
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || M.d[v] != UNR) continue;
            int fr = std::max(freeOther[v], freeMine[v]);
            if (fr > du + 1 && v != headT) continue;
            M.set(v, du + 1);
        }
    }
    cntBFS += M.n;
}

// ------------------------------------------------------------------ self simulation
struct SimState {
    std::vector<int> body;
    int len = 0;
    bool alive = true;
    int eaten = 0;
    int eatenNearKing = 0;   // v15: pearls eaten within P_KING_DROP_R of the king's head (the king's own drops)
    bool headOn = false;
    int headOnId = -1;
};
TileArr<bool> eatenMark;
bool nearKingDrop(int v);

// ------------------------------------------------------------------ v15: sprints through pearls
bool localRulesOn();
// where: P_PSPRINT_WHERE 1 = not on the known round-500 maps (Schooltime, Slithery Fight, Portals, Trauma)
inline bool psOn(int mode) {
    if (!(P_PSPRINT == 1 || P_PSPRINT == mode)) return false;
    if (P_PSPRINT_WHERE == 1 && dbMap >= 0 && !localRulesOn()) return false;
    return true;
}
// A move of k steps: every step after the first needs length >= 3 before it and costs a segment, but a
// pearl eaten on that step gives the segment back (+1 eaten, -1 extra pop). So a dragon on a line of pearls
// strikes as far as the line goes: a 2-long dragon that eats on its first step can go on for free. The
// ladder's top team struck from beyond length-1 tiles 107 times in 20 games; v14 never did, and its threat
// model assumed every enemy reaches only length-1 tiles.
// psSearch fills M with the fewest steps after which a dragon of length L with its head at `from` arrives on
// each tile (the arrival tile may be occupied: a strike), within maxSteps. Visible pearls only.
// With `own` set (my own sprint), my body counts as blocked except the tail from the second step on.
static TileArr<int8_t> psMemoLen[9];
static std::vector<int> psTouched;
static std::vector<int> psPath, psBest;   // (psBest unused by the reach map)
static int psTarget = -1;
static DistMap* psOut = nullptr;
static bool psOwn = false;
static int psMax = 0;
static int psTail = -1;
void psDFS(int u, int j, int len) {
    // j = steps taken so far; len = length now
    if (j >= psMax) return;
    if (j >= 1 && len < 3) return;   // no further step allowed
    for (int d = 0; d < 4; d++) {
        int v = nb[u][d];
        if (v < 0) continue;
        bool onPath = false;
        for (int k = 0; k < (int)psPath.size(); k++) if (psPath[k] == v) { onPath = true; break; }
        if (onPath) continue;
        int nj = j + 1;
        // arrival on v at step nj (a strike on whatever is there)
        if (psOut && psOut->d[v] > nj) { if (psOut->d[v] == UNR) psOut->set(v, nj); else psOut->d[v] = (int16_t)nj; }
        // passing through v needs it free
        bool blocked = false;
        if (occId[v] >= 0) {
            if (psOwn && occId[v] == myId) blocked = !(v == psTail && nj >= 2);
            else blocked = true;
        }
        if (blocked) continue;
        bool pearl = pearlNow[v];
        int nl = (j == 0) ? len + (pearl ? 1 : 0) : len - 1 + (pearl ? 1 : 0);
        if (nl < 2) continue;
        int8_t& m = psMemoLen[nj][v];
        if (m >= nl) continue;
        if (m == 0) psTouched.push_back(nj * MAXN + v);
        m = (int8_t)nl;
        psPath.push_back(v);
        psDFS(v, nj, nl);
        psPath.pop_back();
    }
}
void psClearMemo() {
    for (int x : psTouched) psMemoLen[x / MAXN][x % MAXN] = 0;
    psTouched.clear();
}
// reach map of another dragon (strike tiles by fewest steps)
void psReach(DistMap& M, int from, int L, int maxSteps) {
    M.clear();
    psOut = &M; psOwn = false; psTarget = -1; psMax = std::min(8, maxSteps); psPath.clear(); psBest.clear();
    psDFS(from, 0, L);
    psClearMemo(); psOut = nullptr;
}
// my own sprint path to a target head (directions), empty if none
std::vector<int> psAttackPath(int target, int maxSteps) {
    psOut = nullptr; psOwn = true; psTarget = target; psMax = std::min(8, maxSteps); psPath.clear(); psBest.clear();
    psTail = myBody.empty() ? -1 : myBody.back();
    struct Rec { static bool go(int u, int j, int len, std::vector<int>& ds, std::vector<int>& best) {
        if (j >= psMax) return false;
        if (j >= 1 && len < 3) return false;
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0) continue;
            bool onPath = (v == headT);
            for (int k = 0; k < (int)psPath.size() && !onPath; k++) if (psPath[k] == v) onPath = true;
            if (onPath) continue;
            int nj = j + 1;
            if (v == psTarget) {
                if (best.empty() || (int)best.size() > nj) { best = ds; best.push_back(d); }
                continue;
            }
            if (!best.empty() && (int)best.size() <= nj + 1) continue;
            bool blocked = false;
            if (occId[v] >= 0) blocked = (occId[v] == myId) ? !(v == psTail && nj >= 2) : true;
            if (blocked) continue;
            bool pearl = pearlNow[v];
            int nl = (j == 0) ? len + (pearl ? 1 : 0) : len - 1 + (pearl ? 1 : 0);
            if (nl < 2) continue;
            int8_t& m = psMemoLen[nj][v];
            if (m >= nl) continue;
            if (m == 0) psTouched.push_back(nj * MAXN + v);
            m = (int8_t)nl;
            psPath.push_back(v); ds.push_back(d);
            go(v, nj, nl, ds, best);
            psPath.pop_back(); ds.pop_back();
        }
        return !best.empty();
    } };
    std::vector<int> ds, best;
    Rec::go(headT, 0, myLen, ds, best);
    psClearMemo();
    return best;
}

// v15: a long dragon's move model and survival count a pearl that may lie on a tile it cannot see
// (model probability >= P_SURV_MAYBE): eating it keeps the tail where it is. On Portals our kings
// tail-chased through a 2x2 box behind a portal, ate a pearl there they had not seen and were shut in.
inline void ensureModel(int t);
bool maybePearl(int v) {
    if (P_SURV_MAYBE <= 0 || myLen < P_SURV_MAYBE_LEN || visible[v] || pearlNow[v]) return false;
    if (P_SURV_MAYBE_WHERE == 1 && !onPortalsMap()) return false;
    ensureModel(v);
    return pvNow[v] >= P_SURV_MAYBE;
}
SimState simulate(const std::vector<int>& dirs) {
    cntSim++;
    SimState s; s.body = myBody; s.len = myLen;
    std::vector<int> eatenList;
    for (int j = 0; j < (int)dirs.size(); j++) {
        if (j >= 1 && s.len < 3) { s.alive = false; break; }
        int v = nb[s.body[0]][dirs[j]];
        if (v < 0) { s.alive = false; break; }
        bool self = false;
        for (int t : s.body) if (t == v) { self = true; break; }
        if (!self && occId[v] == myId && freeMine[v] == 0) self = true;   // my own segment outside the known chain
        if (self) { s.alive = false; break; }
        if (occId[v] >= 0 && occId[v] != myId) {
            s.alive = false;
            if (occHead[v]) { s.headOn = true; s.headOnId = occId[v]; }
            break;
        }
        s.body.insert(s.body.begin(), v);
        bool eat = pearlNow[v] && !eatenMark[v];
        bool grow = !eat && !eatenMark[v] && maybePearl(v);
        if (eat) {
            eatenMark[v] = true; eatenList.push_back(v); s.eaten++; s.len++;
            if (P_KING_DROP_PEN > 0 && nearKingDrop(v)) s.eatenNearKing++;
        }
        else if (grow) { eatenMark[v] = true; eatenList.push_back(v); s.len++; }
        else if ((int)s.body.size() > 1) s.body.pop_back();
        if (j >= 1) { if ((int)s.body.size() > 1) s.body.pop_back(); s.len--; }
    }
    for (int t : eatenList) eatenMark[t] = false;
    return s;
}

// ------------------------------------------------------------------ survival DFS
TileArr<int16_t> visitStep(1, -32000);
long dfsNodes, dfsLimit;
bool lastSurvCut = false;   // the last survival() gave up on its node budget (its answer is a guess)
const int16_t* extraFree = nullptr;
inline int blockedUntil(int v) {
    int f = freeOther[v];
    if (tailGrowMode() == 3 && freeGrow[v] > f) f = freeGrow[v];
    if (extraFree && extraFree[v] > f) f = extraFree[v];
    return f;
}
inline bool bodyBlocks(int v, int step, int len) {
    return visitStep[v] > -30000 && visitStep[v] >= step - len + 1;
}

int survDFS(int pos, int step, int len, int depth) {
    if (step >= depth) return step;
    if (++dfsNodes > dfsLimit) return depth;  // give up: optimistic
    int cand[4], score[4], nc = 0;
    for (int d = 0; d < 4; d++) {
        int v = nb[pos][d];
        if (v < 0) {
            // v17 (unknown maps): a portal whose other end nobody has seen yet is a way out, not a wall
            if (P_SURV_UNKPORTAL && viaPortal[pos][d] && step + 1 >= P_SURV_UNKPORTAL_MIN) return depth;
            continue;
        }
        if (blockedUntil(v) > step + 1) continue;
        if (bodyBlocks(v, step, len)) continue;
        int sc = 0;
        for (int e = 0; e < 4; e++) {
            int w = nb[v][e];
            if (w >= 0 && w != pos && blockedUntil(w) <= step + 2 && !bodyBlocks(w, step + 1, len)) sc++;
        }
        cand[nc] = v; score[nc] = sc; nc++;
    }
    for (int i = 0; i < nc; i++)
        for (int j = i + 1; j < nc; j++)
            if (score[j] > score[i]) { std::swap(score[i], score[j]); std::swap(cand[i], cand[j]); }
    int best = step;
    for (int i = 0; i < nc; i++) {
        int v = cand[i];
        int nlen = len;
        bool eat = (pearlNow[v] || maybePearl(v)) && !eatenMark[v];
        if (eat) { eatenMark[v] = true; nlen++; }
        int16_t save = visitStep[v];
        visitStep[v] = (int16_t)(step + 1);
        int r = survDFS(v, step + 1, nlen, depth);
        visitStep[v] = save;
        if (eat) eatenMark[v] = false;
        if (r > best) best = r;
        if (best >= depth) break;
    }
    return best;
}

// Steps survivable from `body` (head placed at step0). Returns reached step index.
long turnNodes = 0;   // survival-search nodes used this turn (bounded to keep the CPU cost safe)
int survival(const std::vector<int>& body, int len, int step0, int depth, long limit, const int16_t* extra = nullptr) {
    // bigger maps cost more per turn outside the search (loops over all tiles): leave them less search
    // (v9: 64x64 maps count as big too, and once the turn's budget is spent every further search gets
    // only P_SURV_FLOOR nodes: on the judge a 64x64 king turn reached 76M points, 52M of them in 80,000 nodes)
    static const long budget = N > 16384 ? (long)(P_TURN_DFS_BUDGET * 0.45) : (N > 4096 || (P_BUDGET_64_BIG && N >= 4096)) ? (long)(P_TURN_DFS_BUDGET * 0.7) : (long)P_TURN_DFS_BUDGET;
    limit = std::min(limit, std::max<long>(P_SURV_FLOOR, budget - turnNodes));
    extraFree = extra;
    int L = body.size();
    for (int i = L - 1; i >= 0; i--) visitStep[body[i]] = (int16_t)(step0 - i);
    dfsNodes = 0; dfsLimit = limit;
    int r = survDFS(body[0], step0, len, depth);
    turnNodes += std::min(dfsNodes, dfsLimit);
    lastSurvCut = dfsNodes > dfsLimit;
    cntSurv++; if (lastSurvCut) cntCut++;
    for (int i = 0; i < L; i++) visitStep[body[i]] = -32000;
    extraFree = nullptr;
    return r;
}

// ------------------------------------------------------------------ threat model
// ------------------------------------------------------------------ gossip (sonar knowledge sharing)
struct Sighting { int tile, round, len, id; };
std::vector<Sighting> sightings;   // enemy heads seen (directly or via gossip), latest per id
int bigEnemySeen = 0;   // v7: longest enemy dragon ever sighted
int gossipCtr = 0;
int lastSplitRound = -9;   // v7: the round of my last split (map gossip for the child)
constexpr uint64_t G_SECRET = 0xC0FFEE1234ABCDULL;
constexpr int GT_SYM = 1, GT_PORTAL = 2, GT_ENEMY = 3, GT_FARM = 4, GT_BIG = 5;
Sighting bestBig = {-1, -1, 0, -1};   // largest ally advertised (tile, round, len, id)
constexpr int GT_KING = 6;
constexpr int GT_BODY = 7;
constexpr int GT_CD = 8;   // a rich tile's countdown as seen by an ally
constexpr int GT_RV = 9;   // v5: the team's endgame meeting tile (tile, round it was chosen)
int rvTile = -1, rvRound = -1;        // v5: endgame rendezvous (persistent per dragon)
constexpr int GT_MAP = 11;   // v7: the known map I play on (index, variant flag): children adopt it at once
constexpr int GT_HOME = 10;  // v5: spawn tile of our lowest-id original dragon (tile, id)
int homeTile = -1, homeId = -1;       // v5: persistent per dragon
Sighting kingK = {-1, -1, 0, -1};     // latest known position of our king (tile, round, len, id)
bool iAmKing = false;
bool killMode = false;
bool loneHunt = false;   // v11: this turn I hunt the enemy's lone king
bool kingInit = false;
bool isKingFlag = false;
bool queenNow = false;   // v18: dragon 0/1 is the team's queen; round 500 is decided on queen length first
int lastContact = -1;                 // last round any enemy part was seen (directly or via gossip)
inline uint64_t mix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}
// the tag also depends on my team, so messages from the other team (e.g. an older copy of this bot
// in a local test) are rejected instead of being read as gossip from allies
inline uint64_t gTag(int type, uint64_t payload) { return mix64(payload ^ ((uint64_t)type << 52) ^ G_SECRET ^ ((uint64_t)(unsigned char)myTeam << 56)) >> 54; }
inline uint64_t gEncode(int type, uint64_t payload) {
    payload &= (1ULL << 50) - 1;
    return ((uint64_t)type << 60) | (gTag(type, payload) << 50) | payload;
}
inline bool gDecode(uint64_t m, int& type, uint64_t& payload) {
    type = (int)(m >> 60);
    payload = m & ((1ULL << 50) - 1);
    return type != 0 && ((m >> 50) & 1023) == gTag(type, payload);
}

void noteSighting(int tile, int round, int len, int id) {
    if (round > lastContact) lastContact = round;
    for (auto& g : sightings)
        if (g.id == id) {
            if (round > g.round || (round == g.round && len > g.len)) { g.tile = tile; g.round = round; g.len = len; }
            return;
        }
    sightings.push_back({tile, round, len, id});
    if (sightings.size() > 40) {
        auto it = std::min_element(sightings.begin(), sightings.end(), [](const Sighting& a, const Sighting& b) { return a.round < b.round; });
        sightings.erase(it);
    }
}

void receiveGossip() {
    for (uint64_t m : msgs) {
        int type; uint64_t pl;
        if (!gDecode(m, type, pl)) continue;
        if (dbMap >= 0 && (type == GT_SYM || type == GT_PORTAL)) continue;
        if (type == GT_MAP) {
            // v7: an ally (usually my parent, right after the split) knows the map: take it if my view fits
            int m = (int)(pl & 255); bool variant = (pl >> 8) & 1;
            int sd = (int)((pl >> 9) & 3);
            if (P_SIDE_DETECT && sd && sideIdx < 0 && firstRound > 0) sideIdx = sd - 1;   // v10: my parent knows our side
#ifdef DEBUGLOG
            if (getenv("SIDE_DEBUG")) fprintf(stderr, "SIDERECV r%d id%d sd%d side%d first%d\n", rnd, myId, sd, sideIdx, firstRound);
#endif
            if (!P_MAP_GOSSIP || dbTried || m >= NMAPDB || m >= 32 || !(dbCandW >> m & 1) || !dbConsistent(m, true)) continue;
            if (variant) { if (dbMap >= 0) dropDB(); applyDB(m); dropDBKeepWalls(m); dbTried = true; }
            else if (dbConsistent(m)) { std::vector<int> x = dbExtraNew; if (dbMap != m) { if (dbMap >= 0) dropDB(); applyDB(m); } dbExtraNew = x; noteExtras(m); if (!P_DB_SPAWNS) dropDBKeepWalls(m); dbTried = true; }
            continue;
        }
        if (type == GT_SYM) {
            int k = (int)(pl & 3);
            if (k < 3 && symKnown < 0 && symAlive[k]) adoptSymmetry(k);
        } else if (type == GT_PORTAL) {
            int ca = (int)(pl & 0x1FFFF), cb = (int)((pl >> 17) & 0x1FFFF), id = (int)((pl >> 34) & 0xFFFF);
            if ((ca % MAXN) >= N || (cb % MAXN) >= N || ca == cb) continue;
            if (portalPartner(id, ca) < 0) { addPortal(id, ca); addPortal(id, cb); }
        } else if (type == GT_ENEMY) {
            int tile = (int)(pl & 0xFFFF), round = (int)((pl >> 16) & 511), len = (int)((pl >> 25) & 255), id = (int)((pl >> 33) & 0xFFFF);
            if (tile >= N || round > rnd) continue;
            noteSighting(tile, round, len, id);
        } else if (type == GT_BIG) {
            int tile = (int)(pl & 0xFFFF), round = (int)((pl >> 16) & 511), len = (int)((pl >> 25) & 511), id = (int)((pl >> 34) & 0xFFFF);
            if (tile >= N || round > rnd || id == myId) continue;
            if (bestBig.id < 0 || round > bestBig.round + 3 || len > bestBig.len || (id == bestBig.id && round > bestBig.round))
                bestBig = {tile, round, len, id};
        } else if (type == GT_KING) {
            // the king is simply the longest dragon of the team that anyone has heard of recently
            int tile = (int)(pl & 0xFFFF), round = (int)((pl >> 16) & 511), len = (int)((pl >> 25) & 511), id = (int)((pl >> 34) & 0xFFFF);
            if (tile >= N || round > rnd || id == myId || round < rnd - kingFreshN()) continue;
            bool better = kingK.id < 0 || kingK.round < rnd - kingFreshN() ||
                          (id == kingK.id ? round > kingK.round : (len > kingK.len || (len == kingK.len && id < kingK.id)));
            if (better) kingK = {tile, round, len, id};
        } else if (type == GT_HOME) {
            int tile = (int)(pl & 0xFFFF), id = (int)((pl >> 16) & 0xFFFF);
            if (tile >= N) continue;
            if (homeId < 0 || id < homeId) { homeTile = tile; homeId = id; }
        } else if (type == GT_RV) {
            int tile = (int)(pl & 0xFFFF), round = (int)((pl >> 16) & 1023);
            if (tile >= N || round > rnd) continue;
            // one meeting tile for the whole team: the earliest chosen one wins (ties: lowest tile)
            if (rvTile < 0 || round < rvRound || (round == rvRound && tile < rvTile)) { rvTile = tile; rvRound = round; }
        } else if (type == GT_BODY) {
            if (turnCount > 0) continue;
            int start = (int)(pl & 127), n = (int)((pl >> 7) & 31), hash = (int)((pl >> 12) & 63);
            std::vector<int> dirs;
            dirs.push_back(hash);   // first entry: 6-bit hash of the anchor tile
            for (int i = 0; i < n && i < 16; i++) dirs.push_back((int)((pl >> (18 + 2 * i)) & 3));
            bodyChunks.push_back({start, dirs});
        } else if (type == GT_CD) {
            int tile = (int)(pl & 0xFFFF), r9 = (int)((pl >> 16) & 511), cd = (int)((pl >> 25) & 4095), cmax = (int)((pl >> 37) & 4095);
            bool pearl = (pl >> 49) & 1;
            if (tile >= N) continue;
            int r0 = rnd - ((rnd - r9) & 511);   // the round it was seen (within the last 512)
            TileK& K = tk[tile];
            if (cmax > K.cdMax) K.cdMax = (int16_t)cmax;
            if (r0 > K.cdR && r0 > K.pR) { noteAttempt(K, rnd, true); K.cdR = (int16_t)r0; K.cdV = (int16_t)cd; K.pR = (int16_t)r0; K.pV = pearl; }
        } else if (type == GT_FARM) {
            int tile = (int)(pl & 0xFFFF), gap = (int)((pl >> 16) & 255);
            if (tile >= N || gap <= 0) continue;
            if (tk[tile].gN == 0) { tk[tile].gN = 1; tk[tile].gSum = gap; tk[tile].gMin = gap; tk[tile].gMax = gap; }
        }
    }
}

bool isolated() {
    if (P_ISO_ROUNDS <= 0) return false;
    int since = rnd - std::max(lastContact, firstRound >= 0 ? std::min(firstRound, 0) : 0);
    return rnd >= P_ISO_MIN_ROUND && since >= P_ISO_ROUNDS;
}

// where does a sonar ray sent in direction d after this turn's move end? 3 = an ally I can see,
// 1 = out of view (unknown), 0 = a wall, an enemy or my own body
int sonarScore(int d) {
    int h = lastPath.empty() ? headT : lastPath.back();
    int facing = lastDirs.empty() ? myFacing : lastDirs.back();
    if (myLen >= 2 && d == (facing + 2) % 4) return 1;   // this ray starts at my tail
    static TileArr<int16_t> mine; static TileArr<int> stamp; static int ctr = 0;
    ctr++;
    // my body after the move: the new path (newest first) and then my old body
    int n = 0;
    for (int i = (int)lastPath.size() - 1; i >= 0 && n < myLen; i--, n++) { stamp[lastPath[i]] = ctr; mine[lastPath[i]] = 1; }
    for (size_t i = 0; i < myBody.size() && n < myLen; i++, n++) { stamp[myBody[i]] = ctr; mine[myBody[i]] = 1; }
    int cur = h;
    for (int k = 0; k < W + H; k++) {
        int nx = nb[cur][d];
        if (nx < 0) return viaPortal[cur][d] ? 1 : 0;
        cur = nx;
        if (stamp[cur] == ctr && mine[cur]) return 0;
        if (!visible[cur]) return 1;
        if (occId[cur] >= 0 && occId[cur] != myId) {
            for (auto& p : vparts) if (p.id == occId[cur]) return p.team == myTeam ? 3 : 0;
            return 0;
        }
    }
    return 1;
}

// choose up to 4 messages for this turn
void sendGossip() {
    std::vector<uint64_t> fresh, pool;
    // fresh sightings first (seen within 2 rounds), biggest first
    std::vector<Sighting> fs;
    for (auto& g : sightings) if (g.round >= rnd - 2) fs.push_back(g);
    std::sort(fs.begin(), fs.end(), [](const Sighting& a, const Sighting& b) { return a.round != b.round ? a.round > b.round : a.len > b.len; });
    for (size_t i = 0; i < fs.size() && i < 2; i++)
        fresh.push_back(gEncode(GT_ENEMY, (uint64_t)fs[i].tile | ((uint64_t)(fs[i].round & 511) << 16) |
                                             ((uint64_t)std::min(fs[i].len, 255) << 25) | ((uint64_t)(fs[i].id & 0xFFFF) << 33)));
    if (symKnown >= 0) pool.push_back(gEncode(GT_SYM, (uint64_t)symKnown));
    if (P_MAP_GOSSIP && dbTried && (dbMap >= 0 || dbWallsOf >= 0)) {
        uint64_t mm = gEncode(GT_MAP, (uint64_t)(dbMap >= 0 ? dbMap : dbWallsOf) | ((uint64_t)(dbMap < 0) << 8) |
                                          ((uint64_t)(P_SIDE_DETECT && sideIdx >= 0 ? sideIdx + 1 : 0) << 9));
        if (rnd - lastSplitRound <= 1) fresh.push_back(mm); else pool.push_back(mm);   // my newborn child needs it now
    }
    for (size_t i = 0; i < portals.size(); i++) {
        int pb = portalPartner(portals[i].id, portals[i].code);
        if (pb < 0 || pb < portals[i].code) continue;
        if (portals[i].id < 0 || portals[i].id > 0xFFFF) continue;
        pool.push_back(gEncode(GT_PORTAL, (uint64_t)portals[i].code | ((uint64_t)pb << 17) | ((uint64_t)portals[i].id << 34)));
    }
    int nf = 0;
    for (int t = 0; t < N && nf < 24; t++)
        if (tk[t].gN > 0 && tk[t].gMax <= 3) {
            int gap = std::max(1, (int)std::lround((double)tk[t].gSum / tk[t].gN));
            pool.push_back(gEncode(GT_FARM, (uint64_t)t | ((uint64_t)std::min(gap, 255) << 16)));
            nf++;
        }
    if (P_GOSSIP_CD) {
        // rich tiles I have seen recently: their countdowns tell allies when pearls appear there
        std::vector<std::pair<int, int>> rich;
        for (int t = 0; t < N; t++) {
            const TileK& K = tk[t];
            if (K.dR < 0 || K.dR < rnd - P_RICH_AGE || K.cdMax <= 0 || K.cdMax > P_RICH_GAP || K.dV < 0) continue;
            rich.push_back({K.cdMax, t});
        }
        std::sort(rich.begin(), rich.end());
        for (size_t i = 0; i < rich.size() && (int)i < P_RICH_MSGS; i++) {
            int t = rich[i].second; const TileK& K = tk[t];
            pool.push_back(gEncode(GT_CD, (uint64_t)t | ((uint64_t)(K.dR & 511) << 16) | ((uint64_t)std::min<int>(K.dV, 4095) << 25) |
                                              ((uint64_t)std::min<int>(K.cdMax, 4095) << 37) | ((uint64_t)(K.pR == K.dR && K.pV) << 49)));
        }
    }
    for (auto& g : sightings)
        if (g.round < rnd - 2 && g.round >= rnd - 20)
            pool.push_back(gEncode(GT_ENEMY, (uint64_t)g.tile | ((uint64_t)(g.round & 511) << 16) |
                                                 ((uint64_t)std::min(g.len, 255) << 25) | ((uint64_t)(g.id & 0xFFFF) << 33)));
    // endgame: advertise the biggest known ally (myself if I am big)
    if (rnd >= cfg.feedStart - P_BIG_LEAD || (isolated() && rnd >= P_ISO_FEED_START - P_BIG_LEAD)) {
        Sighting b = bestBig;
        if (b.id >= 0 && b.round < rnd - 3) b.id = -1;
        if (myLen >= P_BIG_MINLEN && (b.id < 0 || myLen >= b.len)) b = {headT, rnd, myLen, myId};
        if (b.id >= 0)
            fresh.insert(fresh.begin(), gEncode(GT_BIG, (uint64_t)b.tile | ((uint64_t)(b.round & 511) << 16) |
                                                              ((uint64_t)std::min(b.len, 511) << 25) | ((uint64_t)(b.id & 0xFFFF) << 34)));
        if (fresh.size() > 3) fresh.resize(3);
    }
    if (P_KING) {
        Sighting k = kingK;
        if (iAmKing) k = {headT, rnd, myLen, myId};
        if (k.id >= 0 && k.round >= rnd - P_KING_RELAY_AGE) {
            fresh.insert(fresh.begin(), gEncode(GT_KING, (uint64_t)k.tile | ((uint64_t)(k.round & 511) << 16) |
                                                              ((uint64_t)std::min(k.len, 511) << 25) | ((uint64_t)(k.id & 0xFFFF) << 34)));
            if (fresh.size() > 3) fresh.resize(3);
        }
    }
    if (rvMode() == 1 && rvTile >= 0) {
        fresh.insert(fresh.begin(), gEncode(GT_RV, (uint64_t)rvTile | ((uint64_t)(rvRound & 1023) << 16)));
        if (fresh.size() > 3) fresh.resize(3);
    }
    if (rvMode() == 2 && homeTile >= 0 && dbMap < 0) {
        uint64_t hm = gEncode(GT_HOME, (uint64_t)homeTile | ((uint64_t)(homeId & 0xFFFF) << 16));
        if (rnd >= cfg.feedPullStart - P_RV_LEAD || turnCount < 3) { fresh.insert(fresh.begin(), hm); if (fresh.size() > 3) fresh.resize(3); }
        else pool.insert(pool.begin(), hm);
    }
    std::vector<uint64_t> outMsgs = fresh;
    if (!pool.empty()) {
        int start = gossipCtr % (int)pool.size();
        for (int i = 0; i < (int)pool.size() && outMsgs.size() < 4; i++) outMsgs.push_back(pool[(start + i) % pool.size()]);
        gossipCtr += 4 - (int)fresh.size();
    }
    char tmp[64];
    probeDir = -1;
    if (P_PORTAL_PROBE && bodyMsgs.empty()) {
        int h = lastPath.empty() ? headT : lastPath.back();
        int facing = lastDirs.empty() ? myFacing : lastDirs.back();
        for (int d = 0; d < 4 && probeDir < 0; d++) {
            if (myLen >= 2 && d == (facing + 2) % 4) continue;   // that ray would start at my tail
            int x = nb[h][d];
            if (x < 0 || !viaPortal[h][d] || cheb(h, x) <= 3) continue;
            int L = 1, c = x; bool ok = true;
            while (L <= P_PROBE_MAXLEN) {
                int n2 = nb[c][d];
                if (n2 < 0) { if (viaPortal[c][d]) ok = false; break; }
                c = n2; L++;
            }
            if (!ok || L > P_PROBE_MAXLEN) continue;   // a long line: a hit would say little about the exit
            probeDir = d; probeFrom = h; probeRound = rnd;
        }
        if (probeDir >= 0) {
            snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[probeDir], (unsigned long long)(outMsgs.empty() ? 0ULL : outMsgs[0]));
            emit(tmp);
            return;
        }
    }
    if (!bodyMsgs.empty()) {
        // this turn the sonar carries my child's body layout on the rays that reach it
        bool used[4] = {false, false, false, false};
        for (auto& bm : bodyMsgs) {
            snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[bm.first], (unsigned long long)bm.second);
            emit(tmp); used[bm.first] = true;
        }
        size_t k = 0;
        for (int d = 0; d < 4 && k < outMsgs.size(); d++)
            if (!used[d]) { snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[d], (unsigned long long)outMsgs[k++]); emit(tmp); }
        bodyMsgs.clear();
        return;
    }
    if (P_SIDE_DETECT && rnd == lastSplitRound && sideIdx >= 0 && myLen >= 2 && (dbMap >= 0 || dbWallsOf >= 0)) {
        // v10: I split this turn: the ray from my tail hits my child's head; it carries the map and our side
        int back = (myFacing + 2) % 4;
        uint64_t mm = gEncode(GT_MAP, (uint64_t)(dbMap >= 0 ? dbMap : dbWallsOf) | ((uint64_t)(dbMap < 0) << 8) | ((uint64_t)(sideIdx + 1) << 9));
        snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[back], (unsigned long long)mm);
        emit(tmp);
#ifdef DEBUGLOG
        if (getenv("SIDE_DEBUG")) fprintf(stderr, "SIDESEND r%d id%d side%d dir%c\n", rnd, myId, sideIdx, DCH[back]);
#endif
        size_t k = 0;
        for (int d = 0; d < 4 && k < outMsgs.size(); d++)
            if (d != back) { snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[d], (unsigned long long)outMsgs[k++]); emit(tmp); }
        return;
    }
    if (P_SONAR_AIM) {
        // send the most important messages along the rays that reach an ally
        int order[4] = {0, 1, 2, 3}, sc[4];
        for (int d = 0; d < 4; d++) sc[d] = sonarScore(d) * 8 + ((d - rnd) & 3);
        std::sort(order, order + 4, [&](int a, int b) { return sc[a] > sc[b]; });
        for (size_t i = 0; i < outMsgs.size() && i < 4; i++) {
            snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[order[i]], (unsigned long long)outMsgs[i]);
            emit(tmp);
        }
        return;
    }
    for (size_t i = 0; i < outMsgs.size() && i < 4; i++) {
        snprintf(tmp, sizeof tmp, "SONAR %c %llu", DCH[(i + rnd) & 3], (unsigned long long)outMsgs[i]);
        emit(tmp);
    }
}

struct Threat {
    int id, head, neck, len, reach;
    bool lenSure;
    DistMap* dist;
    bool backed = false;   // v12: its heads near it outnumber ours there
    DistMap* ps = nullptr;  // v15: tiles it can strike this turn by a sprint through pearls (fewest steps)
};
std::vector<Threat> enemies, allies;
// distance maps for the dragons in view, created on first use (each holds 6 bytes per tile)
std::vector<DistMap*> threatPool, allyPool, psPool;
inline DistMap* poolMap(std::vector<DistMap*>& pool, int i) {
    while ((int)pool.size() <= i) pool.push_back(new DistMap());
    return pool[i];
}
TileArr<int16_t> pessExtra;
TileArr<int16_t> pessAlly;
// v10: tiles a shorter enemy could strike right after my next move (its sprint reach + 1 step): a king
// that can only go on through such tiles (or a dead end) is one move from being struck (on the ladder a
// 50-long king at round 476 stepped into a pocket whose only exit was in a 4-long striker's reach)
TileArr<int16_t> pessReach;
int pessReachTurn = -1;
std::vector<int> forcedTiles;
TileArr<int16_t> newMine;
TileArr<int16_t> distEnemy, distAlly;
DistMap myDist, candDist, tmpMap;

[[maybe_unused]] double unitValue(int L) { return L + P_UNIT_VALUE; }

// Probability that enemy e strikes a head at distance d (their path length).
double strikeProb(const Threat& e, int myL, int d) {
    int diff = myL - e.len;  // their net gain (in length) from trading
    double p;
    if (diff >= P_BIGDIFF) p = std::max(P_STRIKE_FAV, P_STRIKE_BIGDIFF);
    else if (diff > 0) p = P_STRIKE_FAV;
    else if (diff == 0) p = P_STRIKE_EQ;
    else p = P_STRIKE_UNFAV;
    if (isKingFlag && diff > 0) p = std::max(p, P_KING_STRIKE);
    // v8: on the ladder the top teams strike every longer dragon they can reach (16 of our 10+ long
    // dragons were killed so, 8 of them by one-step strikes of 2-3 long units)
    bool bigTarget = P_LONG_STRIKE_LEN > 0 && myL >= P_LONG_STRIKE_LEN && diff > 0;
    if (bigTarget) p = std::max(p, P_LONG_STRIKE);
    if (d >= 2 && !((isKingFlag || bigTarget) && P_BIG_SPRINT_FULL)) p *= P_STRIKE_SPRINT;
    // v9: on the ladder the strikers that killed our kings and long dragons were 2-6 long and 1-2 steps
    // away; assume such a striker takes its chance against the king or any dragon of P_SMALL_TARGET_LEN+
    if (P_SMALL_STRIKER_LEN > 0 && e.len <= P_SMALL_STRIKER_LEN && d <= 2 && diff > 0 &&
        (isKingFlag || myL >= P_SMALL_TARGET_LEN))
        p = std::max(p, (double)P_SMALL_STRIKE);
    // v12: an enemy with more heads around it than we have there strikes when it does not lose length
    if (P_OUTNUM_P > 0 && e.backed && diff >= 0) p = std::max(p, (double)P_OUTNUM_P * (d >= 2 ? P_STRIKE_SPRINT : 1.0));
    // v18: other teams strike the queen whatever it costs them (she decides round 500); on the ladder ours fell
    // mostly to 2-5 long strikers one or two steps away
    if (queenNow) p = std::max(p, (double)P_QUEEN_STRIKE);
    return p;
}

double threatCost(int v, int myL) {
    double cost = 0;
    for (auto& e : enemies) {
        int d = e.dist->d[v];
        if (d == UNR || d > e.reach) {
            // v15: a sprint through pearls may reach farther
            if (!e.ps || e.ps->d[v] == UNR) continue;
            d = e.ps->d[v];
        }
        if (d == 0) continue;
        double p = strikeProb(e, myL, d);
        double loss = std::max(0, myL - e.len) + P_EQ_LOSS;
        cost += p * loss;
    }
    return cost;
}

// ------------------------------------------------------------------ evaluation
struct Cand {
    std::vector<int> dirs;
    int split = 0;
    double score = -1e18;
    const char* why = "";
};

bool kingVeto = false;   // saw a lower-id original teammate at round 0
bool isKing() {
    if (P_KING == 1) return iAmKing;
    if (P_KING == 2) return firstRound == 0;
    return false;
}

double riskMult() {
    double m = 1.0;
    if (rnd >= P_ENDGAME_ROUND) m *= P_ENDGAME_RISK;
    if (myLen >= P_KING_LEN || (isKing() && (rnd >= cfg.kingSplitUntil || (P_E2_KING_SAFE && rnd >= cfg.feedPullStart - P_E2_LEAD)))) m *= P_KING_RISK;
    if (queenNow) m *= P_QUEEN_RISK;
    return m;
}

// Head field: time-aware BFS from my head labelling each tile with the set of
// first moves that reach it on a shortest path. Goal value per first move G[d].
struct HeadField {
    TileArr<int16_t> d{1, UNR}; TileArr<uint8_t> bits; TileArr<int> list; int n = 0;
    HeadField() {}
    void clear() { for (int i = 0; i < n; i++) { d[list[i]] = UNR; bits[list[i]] = 0; } n = 0; }
};
HeadField hf;
double G[4];
TileArr<double> feedVal;
std::vector<int> feedTiles;
struct BigAlly { int head; int vis; };
std::vector<BigAlly> bigAllies;

inline int freeAt(int v) { return std::max(freeOther[v], freeMine[v]); }

void buildHeadField(int maxD) {
    hf.clear();
    for (int dir = 0; dir < 4; dir++) {
        int v = nb[headT][dir];
        if (v < 0 || freeAt(v) > 1) continue;
        if (hf.d[v] == UNR) { hf.d[v] = 1; hf.bits[v] = (uint8_t)(1 << dir); hf.list[hf.n++] = v; }
        else hf.bits[v] |= (uint8_t)(1 << dir);
    }
    for (int qh = 0; qh < hf.n; qh++) {
        int u = hf.list[qh];
        int du = hf.d[u];
        if (du >= maxD) continue;
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || v == headT) continue;
            if (freeAt(v) > du + 1) continue;
            if (hf.d[v] == UNR) { hf.d[v] = (int16_t)(du + 1); hf.bits[v] = hf.bits[u]; hf.list[hf.n++] = v; }
            else if (hf.d[v] == du + 1) hf.bits[v] |= hf.bits[u];
        }
    }
}

inline bool unknownPortalAt(int t) {
    for (int d = 0; d < 4; d++) if (nb[t][d] < 0 && viaPortal[t][d]) return true;
    return false;
}

inline double tileValue(int t, int dd) {
    if (dd < 0) dd = 0;
    if (dd > 1000) return 0;
    double v = 0;
    if (unknownPortalAt(t)) v = P_PORTAL_TARGET * gpow[dd];
    if (pvNow[t] > 0) v = pvNow[t] * gpow[dd];
    if (spawnIn[t] >= 0) {
        int w = std::max(dd, (int)spawnIn[t] - 1);
        if (w < 1000) v = std::max(v, gpow[w] * 0.9);
    }
    if (farm[t]) v = std::max(v, P_FARM_VALUE * fpow[std::max(dd, 1)]);
    return v;
}

TileArr<int> pmStamp; int pmCounter = 1;
inline void ensureModel(int t) {
    if (pmStamp[t] == pmCounter) return;
    pmStamp[t] = pmCounter;
    pearlModel(t);
}
void buildGoal() {
    for (int d = 0; d < 4; d++) G[d] = 0;
    for (int i = 0; i < hf.n; i++) {
        int t = hf.list[i];
        ensureModel(t);
        int dm = hf.d[t];
        double f = 1.0;
        if (distEnemy[t] < dm) f *= compEnemy();
        if (distAlly[t] < dm) f *= P_COMP_ALLY;
        compF[t] = f;
        double on = tileValue(t, dm - 1), off = tileValue(t, dm + 1);
        if (feedVal[t] > 0) { on += feedVal[t] * gpow[std::max(0, dm - 1)]; off += feedVal[t] * gpow[dm + 1]; }
        if (on <= 0) continue;
        on *= f; off *= f;
        for (int d = 0; d < 4; d++) G[d] += (hf.bits[t] >> d & 1) ? on : off;
    }
}

// ------------------------------------------------------------------ path planner
// Beam search over simple paths from the head: value of a path = discounted expected pearls
// collected (each tile once) + discounted "tail" potential at its end. pathG[d] = best per first move.
double pathG[4];
TileArr<float> phiT;
DistMap* kingDist = nullptr;
bool kingDropOn = false;   // v15: this turn my eating next to the king counts against a move
bool nearKingDrop(int v) {
    if (!kingDropOn) return false;
    if (kingDist) return kingDist->d[v] != UNR && kingDist->d[v] <= P_KING_DROP_R;
    return manhattan(v, kingK.tile) <= P_KING_DROP_R;
}
// non-king dragons leave pearls the king can reach about as fast to the king
double kingDefer(int t, int step) {
    // v7: once the endgame pull has started, every map defers to the king (the elimination-prone
    // maps had no deference at all: feeders ate the pearls dropped for the king)
    bool late = P_E7_DEFER && rnd >= cfg.feedPullStart;
    double f = late ? std::min(cfg.kingDefer, (double)P_E7_DEFER_F) : cfg.kingDefer;
    if (isKingFlag || kingK.id < 0 || f >= 1.0 || (rnd < cfg.kingSplitUntil && !late)) return 1.0;
    if (kingDist) {
        int d = kingDist->d[t];
        return (d != UNR && d <= std::min(step + P_DEFER_SLACK, P_DEFER_RADIUS)) ? f : 1.0;
    }
    int age = rnd - kingK.round;
    if (age <= 3 && manhattan(kingK.tile, t) <= P_DEFER_RADIUS + age) return f;
    return 1.0;
}
// expected reward for arriving at tile t on my move `step` (1 = this turn)
// ---- king safety maps
void buildHome() {
    int hm = dbMap >= 0 ? dbMap : (P_HOME_WALLS ? dbWallsOf : -1);   // v7.1: also on a variant (walls known)
    int ti = mySide();
    if (hm < 0 || (homeFor == hm && homeSide == ti)) return;
    homeFor = hm; homeSide = ti;
    const MapDef& M = MAPDB[hm];
    static TileArr<int16_t> dm0, dm1;
    TileArr<int16_t>* dm[2] = {&dm0, &dm1};
    for (int side = 0; side < 2; side++) {
        std::vector<int> q;
        for (int t = 0; t < N; t++) (*dm[side])[t] = UNR;
        int p = 0;
        for (int i = 0; i < M.ndr; i++) {
            int team = M.drg[p], n = M.drg[p + 1];
            if ((team == ti) == (side == 0)) { (*dm[side])[M.drg[p + 2]] = 0; q.push_back(M.drg[p + 2]); }
            p += 2 + n;
        }
        for (size_t qh = 0; qh < q.size(); qh++) {
            int u = q[qh];
            for (int d = 0; d < 4; d++) {
                int v = nb[u][d];
                if (v < 0 || (*dm[side])[v] != UNR) continue;
                (*dm[side])[v] = (int16_t)((*dm[side])[u] + 1); q.push_back(v);
            }
        }
    }
    for (int t = 0; t < N; t++) homeAdv[t] = (int16_t)std::max(-500, std::min(500, (int)dm1[t] - (int)dm0[t]));
}
TileArr<float> dangerV; TileArr<int> dangerStamp;
std::vector<std::pair<int, int>> dangerSrc;   // (tile, radius)
double dangerAt(int t) {
    if (dangerStamp[t] == pmCounter) return dangerV[t];
    dangerStamp[t] = pmCounter;
    double v = 0;
    for (auto& ds : dangerSrc) {
        int d = manhattan(ds.first, t);
        if (d <= ds.second) v += 1.0 - (double)d / (ds.second + 1);
    }
    if (homeFor >= 0 && homeAdv[t] < 0) v += P_KING_AWAY * std::min(1.0, -homeAdv[t] / 10.0);
    dangerV[t] = (float)v;
    return v;
}

double growMul = 1.0;   // < 1 when I am too long for the room around me (eating more would trap me)
inline double rewardAt(int t, int step) {
    ensureModel(t);
    double p = pvNow[t] * growMul;
    if (spawnIn[t] >= 0 && spawnIn[t] <= step - 1) p = std::max(p, 0.9);
    else if (dbMap >= 0 && spawnIn[t] < 0 && gapB[t] > 0 && p < 1.0 && (P_HAZARD_UNSEEN || tk[t].cdR >= 0)) {
        double hz = 2.0 / (gapA[t] + gapB[t]);
        p += (1.0 - p) * std::min(0.9, (step - 1) * hz) * 0.7;
    }
    if (farm[t] && step >= 2) p = std::max(p, P_FARM_ARRIVE * growMul);
    if (spawnIn[t] >= 0 && spawnIn[t] <= step - 1) p = std::min(p, std::max(pvNow[t], 0.9) * growMul);
    double f = 1.0;
    if (distEnemy[t] < step) f *= compEnemy();
    if (distAlly[t] < step && !isKingFlag) f *= P_COMP_ALLY;
    f *= kingDefer(t, step);
    double r = p * f + feedVal[t];
    if (isKingFlag && P_KING_DANGER > 0) r -= P_KING_DANGER * dangerAt(t);
    return r;
}

void buildPhi() {
    // phi(t) = max_u s(u) * kappa^dist(t,u) over the static graph (hf tiles)
    static std::vector<std::pair<float, int>> heap;
    heap.clear();
    static TileArr<float> base;
    for (int i = 0; i < hf.n; i++) {
        int t = hf.list[i];
        base[t] = (float)rewardAt(t, std::max<int>(2, hf.d[t]));
        phiT[t] = 0;
    }
    for (int i = 0; i < hf.n; i++) {
        int t = hf.list[i];
        // value of a spot = its own pearl plus part of its neighbours' (rich regions attract more)
        double s = base[t];
        if (P_PHI_CLUSTER > 0)
            for (int d = 0; d < 4; d++) { int u = nb[t][d]; if (u >= 0 && hf.d[u] != UNR) s += P_PHI_CLUSTER * std::max(0.0f, base[u]); }
        if (s > 0.02) heap.push_back({(float)s, t});
    }
    phiT[headT] = 0;
    std::make_heap(heap.begin(), heap.end());
    const float kap = (float)P_PHI_KAPPA;
    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end());
        auto [v, t] = heap.back(); heap.pop_back();
        if (v <= phiT[t]) continue;
        phiT[t] = v;
        float nv = v * kap;
        if (nv < 0.01f) continue;
        for (int d = 0; d < 4; d++) {
            int u = nb[t][d];
            if (u < 0 || hf.d[u] == UNR) continue;
            if (nv > phiT[u]) { heap.push_back({nv, u}); std::push_heap(heap.begin(), heap.end()); }
        }
    }
}

struct BN { int tile, par, first, step, len; float g, f; uint64_t sig0, sig1; };
inline void sigAdd(uint64_t& a, uint64_t& b, int t) { uint32_t h = (uint32_t)t * 2654435761u; if (h >> 31) a |= 1ULL << (h & 63); else b |= 1ULL << ((h >> 8) & 63); }
inline bool sigHas(uint64_t a, uint64_t b, int t) { uint32_t h = (uint32_t)t * 2654435761u; return (h >> 31) ? (a >> (h & 63) & 1) : (b >> ((h >> 8) & 63) & 1); }
std::vector<BN> bnodes;
TileArr<int> bestIdx(4), bestStamp(4); int bestCounter = 1;

// ------------------------------------------------------------------ farm pockets
// A pocket is a 1-wide dead-end corridor holding fast-respawning pearls (known maps only). A unit
// walks in, eats everything, and at the dead end splits off all but 2 segments: the child (the old
// tail, reversed) walks out, the 2-long stub dies inside and leaves a pearl for the next visit.
// Net gain per visit = pearls inside - 2, e.g. +4 on Stronghold/Trauma, +1 on Dilemma/Autarky.
struct Pocket { int junction; std::vector<int> path; int farms; double exp; std::vector<int16_t> dist; bool ours; };
std::vector<Pocket> pockets;
TileArr<int16_t> pocketOf;    // 1 + index of the pocket whose path contains the tile (0 = none)
TileArr<int16_t> pocketPos;   // position along that path (0 = first tile after the junction)
int pocketsFor = -2;
int turnSerial = 0, pocketsTurn = -1;
int unitPeak = 0;   // most units my team had while I lived (endgame feeding keeps a share of them)
// all four edges of tile t seen with my own eyes (unknown maps: an unseen edge may be a wall)
inline bool edgesSeen(int t) { return eHd[t] && eVd[t] && eHd[ID(TX(t), TY(t) + 1)] && eVd[ID(TX(t) + 1, TY(t))]; }
// unknown maps: a tile whose spawn gap is known (from what I saw) to be tiny
inline int farmGapOnline(int t) {
    const TileK& K = tk[t];
    if (K.gN > 0) return K.gMax;
    return 0;
}
void buildPockets() {
    bool online = dbMap < 0 && P_POCKET_ONLINE;
    if (online) {
        // rebuilt once per turn from this dragon's own knowledge
        if (pocketsTurn == turnSerial) return;
        pocketsTurn = turnSerial; pocketsFor = -3;
    } else {
        int key = P_SIDE_DETECT ? dbMap * 2 + mySide() : dbMap;   // v10: which pockets are ours depends on our side
        if (pocketsFor == key) return;
        pocketsFor = key;
    }
    pockets.clear();
    for (int t = 0; t < N; t++) { pocketOf[t] = 0; pocketPos[t] = 0; }
    if ((dbMap < 0 && !online) || !P_POCKET) return;
    for (int t = 0; t < N; t++) {
        if (online && (!edgesSeen(t) || !tk[t].gN)) continue;
        int deg = 0, only = -1;
        bool port = false;
        for (int d = 0; d < 4; d++) { if (nb[t][d] >= 0) { deg++; only = nb[t][d]; } if (viaPortal[t][d]) port = true; }
        if (deg != 1 || (online && port)) continue;
        std::vector<int> rev = {t};
        int prev = t, cur = only;
        bool okp = true;
        while (true) {
            int dg = 0, nx = -1;
            for (int d = 0; d < 4; d++) { int v = nb[cur][d]; if (v >= 0) { dg++; if (v != prev) nx = v; } }
            if (online && dg <= 2 && !edgesSeen(cur)) { okp = false; break; }
            if (dg < 2) { okp = false; break; }
            if (dg > 2) break;
            rev.push_back(cur); prev = cur; cur = nx;
            if (rev.size() > 40) { okp = false; break; }
        }
        if (!okp) continue;
        Pocket P; P.junction = cur; P.path.assign(rev.rbegin(), rev.rend());
        P.farms = 0; P.exp = 0;
        for (int u : P.path) {
            int g = online ? farmGapOnline(u) : (gapB[u] > 0 ? gapB[u] : 0);
            if (g > 0 && g <= 10) { P.exp += 1.0; if (g <= 1) P.farms++; }
        }
        if (P.exp < (online ? P_POCKET_ONLINE_MIN_EXP : P_POCKET_MIN_EXP)) continue;
        // distance of every tile to the junction (map only, bodies ignored) for the long-range pull
        P.dist.assign(N, (int16_t)UNR);
        std::vector<int> q = {P.junction};
        P.dist[P.junction] = 0;
        for (size_t qh = 0; qh < q.size(); qh++) {
            int u = q[qh];
            for (int d = 0; d < 4; d++) {
                int v = nb[u][d];
                if (v < 0 || P.dist[v] != UNR) continue;
                P.dist[v] = (int16_t)(P.dist[u] + 1); q.push_back(v);
            }
        }
        if (online) P.ours = true;
        else { buildHome(); P.ours = homeAdv[P.junction] >= 0; }
        pockets.push_back(P);
        for (size_t i = 0; i < P.path.size(); i++) { pocketOf[P.path[i]] = (int16_t)pockets.size(); pocketPos[P.path[i]] = (int16_t)i; }
    }
#ifdef DEBUGLOG
    if (online && getenv("POCKET_DEBUG") && !pockets.empty() && rnd % 10 == 0) {
        fprintf(stderr, "r%d id%d len%d online pockets:", rnd, myId, myLen);
        for (auto& P : pockets) fprintf(stderr, " [end (%d,%d) len %zu exp %.0f]", TX(P.path.back()), TY(P.path.back()), P.path.size(), P.exp);
        fprintf(stderr, "\n");
    }
#endif
}
// On maps without splitting (Stronghold, Trauma) one dragon is the designated farmer and never
// claims the crown; on swarm maps any small unit may farm.
int farmerRole = -1;   // -1 undecided, 0 courier / other, 1 farmer, 2 king lineage (Stronghold/Trauma)
std::vector<int> birthTiles;   // my body as known on my first turn (until the role is decided)
inline bool roleMaps() { return cfg.splitMin >= 999 && cfg.pocketMaxLen > 0; }
inline int pocketMaxLenNow() { return dbMap < 0 && P_POCKET_ONLINE ? P_POCKET_ONLINE_MAXLEN : cfg.pocketMaxLen; }
// On those maps a child learns its role from its length at birth: odd = farmer, 2 mod 4 = courier,
// 0 mod 4 = king lineage. Every split picks its size accordingly.
inline int roleOfLen(int k) { return (k & 1) ? 1 : (k % 4 == 2 ? 0 : 2); }
inline int childRole(int k) {   // role a child of size k should get when I split
    if (farmerRole == 2) return k > myLen - k ? 2 : 0;   // the bigger part carries the crown
    return farmerRole;
}
inline bool kParityOK(int k) { return !roleMaps() || farmerRole < 0 || roleOfLen(k) == childRole(k); }
int birthLen = 0;
void decideRole() {
    if (farmerRole >= 0) return;
    if (turnCount == 0) { birthTiles = myBody; birthLen = myLen; }
    if (dbMap < 0 || (P_ROLE_WAIT_DB && !dbTried)) {   // map not recognised yet (e.g. a child whose view fits two maps): decide later
        if (turnCount > 40) { farmerRole = 0; birthTiles.clear(); }
        return;
    }
    farmerRole = 0;
    std::vector<int> bt; bt.swap(birthTiles);
    if (!roleMaps() || pockets.empty()) return;
    if (firstRound == 0 && dbMap >= 0) {
        // starting dragon: the one sitting in the richest area is the king-designate, the others farm
        const MapDef& M = MAPDB[dbMap];
        int ti = mySide();
        auto richness = [&](int h) {
            static TileArr<int16_t> dd;
            std::vector<int> q = {h};
            for (int t = 0; t < N; t++) dd[t] = -1;
            dd[h] = 0;
            double sum = 0;
            for (size_t qh = 0; qh < q.size(); qh++) {
                int u = q[qh];
                if (gapB[u] > 0) sum += 2.0 / (gapA[u] + gapB[u]);
                if (dd[u] >= 8) continue;
                for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && dd[v] < 0) { dd[v] = dd[u] + 1; q.push_back(v); } }
            }
            return sum;
        };
        // king-designate: richest start; ties go to the start farthest from our pocket
        auto pocketDist = [&](int h) {
            int bd = 1 << 20;
            for (auto& P : pockets) if (P.ours && P.dist[h] != UNR) bd = std::min(bd, (int)P.dist[h]);
            return bd;
        };
        int p = 0, kingStart = -1;
        double kr = -1; int kdist = -1;
        for (int i = 0; i < M.ndr; i++) {
            int team = M.drg[p], n = M.drg[p + 1], h = M.drg[p + 2];
            if (team == ti) {
                double r = std::floor(richness(h) * 1000 + 0.5);
                int pd = pocketDist(h);
                if (r > kr || (r == kr && pd > kdist)) { kr = r; kdist = pd; kingStart = h; }
            }
            p += 2 + n;
        }
        farmerRole = headT != kingStart ? 1 : 2;
#ifdef DEBUGLOG
        if (getenv("ROLE_DEBUG")) fprintf(stderr, "r%d id%d head(%d,%d) richness %.2f best %.2f role %d pockets %zu\n", rnd, myId, TX(headT), TY(headT), richness(headT), kr / 1000, farmerRole, pockets.size());
#endif
    } else {
        // a child's role is written in its length at birth (odd farmer, 2 mod 4 courier, 0 mod 4 king line)
        farmerRole = roleOfLen(birthLen);
#ifdef DEBUGLOG
        if (getenv("ROLE_DEBUG")) fprintf(stderr, "r%d id%d child birth len %d role %d (turn %d)\n", rnd, myId, birthLen, farmerRole, turnCount);
#endif
    }
}
bool farmerOK() {
    return P_POCKET && pocketMaxLenNow() > 0 && !pockets.empty() && !isKingFlag && (int)myBody.size() == myLen &&
           myLen <= pocketMaxLenNow() && unitCount < UNIT_LIMIT && (rnd >= P_POCKET_MIN_ROUND || roleMaps()) &&
           (!roleMaps() || farmerRole == 1) && rnd < cfg.feedPullStart;
}
// the state after a move has its head inside a pocket: can it farm the pocket and split out at the end?
bool pocketViable(const SimState& s) {
    if (!farmerOK()) return false;
    int h = s.body[0];
    int pi = pocketOf[h];
    if (!pi) return false;
    const Pocket& P = pockets[pi - 1];
    int i = pocketPos[h], D = P.path.size();
    int sure = 0;
    for (int j = i + 1; j < D; j++) {
        int u = P.path[j];
        if (occId[u] >= 0 || freeOther[u] > 0) return false;    // someone else is in there
        for (int t : s.body) if (t == u) return false;
        if (!visible[u]) ensureModel(u);
        if (visible[u] ? pearlNow[u] : (dbMap >= 0 ? (gapB[u] > 0 && gapB[u] <= 1 && pvNow[u] >= 0.8) : (farmGapOnline(u) == 1 && rnd - tk[u].pR >= 2))) sure++;
    }
    return s.len + sure >= 4;
}
// head at the end of a pocket (no way on): keep a 2-long stub, the rest leaves as a child
bool pocketSplitNow() {
    int pi = pocketOf[headT];
    if (!pi || (int)myBody.size() != myLen || myLen < 4 || unitCount >= UNIT_LIMIT) return false;
    return true;
}

long beamWork = 0;   // v9: beam expansions this turn (CPU guard, see P_BEAM_WORK)
void planPaths() {
    beamWork = 0;
    for (int d = 0; d < 4; d++) pathG[d] = -1;
    buildPhi();
    const int BW = P_BEAM_W, BD = P_BEAM_D, Q = P_BEAM_Q;
    const double gam = P_PATH_GAMMA;
    double gp[64]; gp[0] = 1; for (int i = 1; i < 64; i++) gp[i] = gp[i - 1] * gam;
    bnodes.clear();
    std::vector<int> level, next;
    // paths ending at the end of a farm pocket stop there (the dragon splits out): keep their value
    double termBest[4] = {-1, -1, -1, -1};
    // (only for the designated farmers of Stronghold/Trauma: on swarm maps it lures units into pockets too often)
    bool farming = roleMaps() && farmerRole == 1 && farmerOK();
    for (int d = 0; d < 4; d++) {
        int v = nb[headT][d];
        if (v < 0 || freeAt(v) > 1) continue;
        double r1 = rewardAt(v, 1);
        double g = r1 * gp[1];
        uint64_t a0 = 0, a1 = 0;
        sigAdd(a0, a1, headT); sigAdd(a0, a1, v);
        bnodes.push_back({v, -1, d, 1, myLen + (r1 >= 0.5 ? 1 : 0), (float)g, (float)(g + gp[1] * P_TAIL_W * phiT[v]), a0, a1});
        level.push_back((int)bnodes.size() - 1);
        pathG[d] = std::max(pathG[d], (double)bnodes.back().f);
    }
    for (int depth = 1; depth < BD && !level.empty(); depth++) {
        next.clear();
        bestCounter++;
        int step = depth + 1;
        for (int ni : level) {
            const BN nd = bnodes[ni];
            for (int d = 0; d < 4; d++) {
                int v = nb[nd.tile][d];
                if (v < 0) continue;
                if (freeOther[v] > step || freeMine[v] > step) continue;
                // revisiting a tile of this path: only once my body has left it (the tail tile is blocked)
                int lastStep = v == headT ? 0 : -1;
                if (lastStep < 0 && sigHas(nd.sig0, nd.sig1, v))
                    for (int p = ni; p >= 0 && lastStep < 0; p = bnodes[p].par) if (bnodes[p].tile == v) lastStep = bnodes[p].step;
                double r;
                if (lastStep >= 0) {
                    if (step <= lastStep + nd.len) continue;
                    // a tile already on this path pays again only if it can respawn in between
                    if (farm[v] && step >= lastStep + nd.len + 2) r = P_FARM_ARRIVE * kingDefer(v, step);
                    else if (dbMap >= 0 && gapB[v] > 0) r = 0.9 * gapCdf(gapA[v], gapB[v], step - lastStep - 1) * kingDefer(v, step);
                    else r = 0.0;
                } else r = rewardAt(v, step);
                beamWork++;
                double g = nd.g + r * gp[step];
                int nlen = nd.len + (r >= 0.5 ? 1 : 0);
                float f = (float)(g + gp[step] * P_TAIL_W * phiT[v]);
                if (farming && pocketOf[v] && pocketPos[v] == (int)pockets[pocketOf[v] - 1].path.size() - 1 && nlen >= 4)
                    termBest[nd.first] = std::max(termBest[nd.first], g + gp[step] * (P_TAIL_W * phiT[v] + P_POCKET_CONT));
                uint64_t a0 = nd.sig0, a1 = nd.sig1;
                sigAdd(a0, a1, v);
                int key = v * 4 + nd.first;
                if (bestStamp[key] == bestCounter) {
                    BN& o = bnodes[bestIdx[key]];
                    if (o.g >= g) continue;
                    o = {v, ni, nd.first, step, nlen, (float)g, f, a0, a1};
                    continue;
                }
                bestStamp[key] = bestCounter;
                bnodes.push_back({v, ni, nd.first, step, nlen, (float)g, f, a0, a1});
                bestIdx[key] = (int)bnodes.size() - 1;
                next.push_back(bestIdx[key]);
            }
        }
        // select top BW by f, with at least Q per first move
        std::sort(next.begin(), next.end(), [](int a, int b) { return bnodes[a].f > bnodes[b].f; });
        level.clear();
        int cnt[4] = {0, 0, 0, 0};
        std::vector<int> rest;
        for (int ni : next) {
            int fm = bnodes[ni].first;
            if ((int)level.size() < BW) { level.push_back(ni); cnt[fm]++; }
            else if (cnt[fm] < Q) { level.push_back(ni); cnt[fm]++; }
        }
        // value of a first move = best f among its paths at the deepest level they reach
        double lv[4] = {-1, -1, -1, -1};
        for (int ni : level) {
            const BN& b = bnodes[ni];
            if (b.f > lv[b.first]) lv[b.first] = b.f;
        }
        for (int d = 0; d < 4; d++) if (lv[d] >= 0) pathG[d] = lv[d];
#ifdef DEBUGLOG
        {
            static int dbgId = getenv("DRAGON_DEBUG") ? atoi(getenv("DRAGON_DEBUG")) : -1;
            if (dbgId == myId && getenv("DRAGON_DEBUG_PATH") && (depth == BD - 1 || level.empty() || (int)bnodes.size() > P_BEAM_NODES)) {
                for (int d = 0; d < 4; d++) {
                    int bi = -1;
                    for (int ni : level) if (bnodes[ni].first == d && (bi < 0 || bnodes[ni].f > bnodes[bi].f)) bi = ni;
                    if (bi < 0) continue;
                    std::string ps;
                    std::vector<int> path;
                    for (int p = bi; p >= 0; p = bnodes[p].par) path.push_back(p);
                    for (int k = (int)path.size() - 1; k >= 0; k--) {
                        const BN& b = bnodes[path[k]];
                        char tmp[64]; snprintf(tmp, sizeof tmp, "(%d,%d:%.2f)", TX(b.tile), TY(b.tile), rewardAt(b.tile, b.step));
                        ps += tmp;
                    }
                    fprintf(stderr, "   path %c g=%.2f f=%.2f phiEnd=%.2f %s\n", DCH[d], bnodes[bi].g, bnodes[bi].f, phiT[bnodes[bi].tile], ps.c_str());
                }
            }
        }
#endif
        if ((int)bnodes.size() > P_BEAM_NODES) break;
    }
    for (int d = 0; d < 4; d++) if (termBest[d] > pathG[d]) pathG[d] = termBest[d];
}

// tiles reachable from `start` (time t0) that no enemy reaches first, capped
bool spaceAllyNow = false;   // v10: the king counts only the room no ally reaches first either
int spaceMargin = 0;         // v14: the king counts only the room it reaches this many moves before any enemy
int spaceCount(int start, int t0, int cap) {
    candDist.clear();
    candDist.set(start, t0);
    int cnt = 1;
    for (int qh = 0; qh < candDist.n && cnt < cap; qh++) {
        int u = candDist.list[qh];
        int du = candDist.d[u];
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || candDist.d[v] != UNR) continue;
            if (freeAt(v) > du + 1) continue;
            candDist.set(v, du + 1);
            if (distEnemy[v] >= du + 1 + spaceMargin && ((!P_SPACE_ALLY && !spaceAllyNow) || distAlly[v] >= du + 1)) cnt++;
        }
    }
    return cnt;
}

// Long dragons: after the move, can the head either follow its own tail (reach a body tile by
// the time it is vacated) or reach enough open space? This cannot be fooled by search limits.
TileArr<int16_t> ownFree;
bool longSafe(const SimState& s) {
    int L = s.len, nb0 = (int)s.body.size();
    for (int i = 0; i < nb0; i++) ownFree[s.body[i]] = (int16_t)(L - i + 1);
    static TileArr<int> q; static TileArr<int16_t> dq; static TileArr<bool> seen;
    int n = 0, cnt = 0, pearlsSeen = 0; bool ok = false;
    q[n] = s.body[0]; dq[n++] = 0; seen[s.body[0]] = true;
    for (int qh = 0; qh < n && !ok; qh++) {
        int u = q[qh], du = dq[qh];
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || seen[v]) continue;
            int arr = du + 1;
            if (freeOther[v] > arr) continue;
            if (ownFree[v] > 0) {
                // tail chase possible (every pearl I may eat on the way keeps the tail in place one more turn)
                if (arr >= ownFree[v] + P_TAIL_SLACK + pearlsSeen && v != s.body[1]) { ok = true; break; }
                continue;
            }
            if (pearlNow[v]) pearlsSeen++;
            seen[v] = true; q[n] = v; dq[n++] = (int16_t)arr;
            if (++cnt >= L + 2) { ok = true; break; }
        }
    }
    for (int i = 0; i < n; i++) seen[q[i]] = false;
    for (int i = 0; i < nb0; i++) ownFree[s.body[i]] = 0;
    return ok;
}

// after a move leading into a dead end: could the tail part still split off and escape later?
// expected pearls in the small region ahead of a head (used for dead-end decisions)
double regionPearls(const std::vector<int>& body) {
    static int q[64]; static int16_t dd[64];
    int n = 0; double sum = 0;
    q[n] = body[0]; dd[n++] = 0;
    for (int qh = 0; qh < n && n < 40; qh++) {
        int u = q[qh];
        for (int d = 0; d < 4 && n < 40; d++) {
            int v = nb[u][d];
            if (v < 0 || freeOther[v] > dd[qh] + 1) continue;
            bool seen = false;
            for (int i = 0; i < n && !seen; i++) if (q[i] == v) seen = true;
            for (int t : body) if (t == v) seen = true;
            if (seen) continue;
            q[n] = v; dd[n++] = (int16_t)(dd[qh] + 1);
            sum += rewardAt(v, dd[qh] + 1);
        }
    }
    return sum;
}
bool rescueViableAfter(const SimState& s) {
    int L = s.len;
    if (unitCount >= UNIT_LIMIT || (int)s.body.size() != L) return false;
    if (L < 4) return L + regionPearls(s.body) >= 5.0;  // short: grows on the pearls ahead, splits at the end
    if (regionPearls(s.body) < P_DEADEND_MINPEARLS) return false;
    int k = L - 2;
    std::vector<int> cb;
    for (int i = L - 1; i >= L - k; i--) cb.push_back(s.body[i]);
    for (int i = 0; i < L - k; i++) newMine[s.body[i]] = BIG_FREE;
    int cdep = std::min(k + 2, P_SURV_DEPTH);
    int cs = survival(cb, k, 0, cdep, 3000, newMine.p);
    for (int i = 0; i < L - k; i++) newMine[s.body[i]] = 0;
    return cs >= cdep;
}

void evaluateMove(Cand& c) {
    SimState s = simulate(c.dirs);
    if (!s.alive) {
        c.score = -1e15; c.why = "dead";
        if (s.headOn) {
            bool ally = false;
            for (auto& a : allies) if (a.id == s.headOnId) ally = true;
            c.score = ally ? -3e15 : -0.5e15;
        }
        return;
    }
    int L = s.len;
    double score = 0;
    // long dragons must be able to keep moving until their whole current body has moved on,
    // otherwise they coil themselves into a trap in confined fields
    bool longOne = isKingFlag || L > P_SURV_DEPTH;
    int depth = std::min(L + 2, longOne ? P_KING_SURV_DEPTH : P_SURV_DEPTH);
    if (P_E2_HORIZON) depth = std::max(1, std::min(depth, 499 - rnd + P_E2_HORIZON_MARGIN));   // the game ends after round 499 (v14: + a margin: every pearl eaten keeps the tail in place one more turn)
    long dlim = c.dirs.size() > 1 ? P_DFS_LIMIT : isKingFlag ? P_KING_DFS_LIMIT : longOne ? P_LONG_DFS_LIMIT : P_DFS_LIMIT;
    if (queenNow) {
        // v18: a short queen checked only L+2 moves of room and walked into Trauma's long dead-end corridors
        depth = std::max(depth, (int)P_QUEEN_SURV_DEPTH);
        if (P_E2_HORIZON) depth = std::max(1, std::min(depth, 499 - rnd + P_E2_HORIZON_MARGIN));
        dlim = std::max(dlim, (long)P_KING_DFS_LIMIT);
    }
    int surv = survival(s.body, L, 1, 1 + depth, dlim) - 1;
    // a long search that ran out of nodes only guessed "safe": double-check with the cheap tail/space test
    if (P_CUT_TRAP > 0 && surv >= depth && lastSurvCut && L >= P_LONG_TRAP_LEN && (!P_E2_HORIZON || 499 - rnd + P_E2_HORIZON_MARGIN > L) && (int)s.body.size() == L && !longSafe(s))
        score -= P_CUT_TRAP;
    for (int f : forcedTiles) if (f == s.body[0]) score -= 5000;
    if (P_LONG_TRAP > 0 && (cfg.splitMin >= 999 || P_LONG_ALLMAPS || (P_E2_LONGSAFE && isKingFlag)) && L >= P_LONG_TRAP_LEN && (!P_E2_HORIZON || 499 - rnd + P_E2_HORIZON_MARGIN > L) && (int)s.body.size() == L && !longSafe(s)) score -= P_LONG_TRAP;
    bool pocketIn = surv < depth && !queenNow && pocketViable(s);   // v18: the queen never farms a pocket (the stub left behind is her)
    if (pocketIn) score -= P_POCKET_COST;
    else if (surv < depth) {
        // a dead end is acceptable if the tail can later split off and escape (corridor farming)
        if (P_DEADEND && cfg.splitMin < 999 && rnd >= P_DEADEND_MIN_ROUND && !isKingFlag && !queenNow && rescueViableAfter(s)) score -= cfg.deadEndCost;
        else score -= std::max((double)P_DEADEND_FLOOR, 20000.0 - 500.0 * surv);   // v9: floor (a king with 40+ moves of room scored a bonus)
    } else {
        bool pessFail = false;
        if (!enemies.empty()) {
            int sp = survival(s.body, L, 1, 1 + depth, dlim / 2, pessExtra.p) - 1;
            if (sp < depth) { score -= P_W_PESS; pessFail = true; }
        }
        if (!allies.empty() && !pessFail) {
            int sp = survival(s.body, L, 1, 1 + depth, dlim / 2, pessAlly.p) - 1;
            if (sp < depth) score -= P_W_PESS_ALLY;
        }
        if (P_KING_PESS2 > 0 && !pessFail && !enemies.empty() && (isKingFlag || L >= P_KING_PESS2_LEN)) {
            if (pessReachTurn != turnSerial) {
                pessReachTurn = turnSerial;
                for (int t = 0; t < N; t++) pessReach[t] = 0;
                for (auto& e : enemies) {
                    if (e.len >= myLen) continue;
                    for (int i = 0; i < e.dist->n; i++) {
                        int t = e.dist->list[i];
                        if (e.dist->d[t] <= e.reach + 1) pessReach[t] = 3;   // blocked for my arrival at step 2
                    }
                    if (e.ps) for (int i = 0; i < e.ps->n; i++) pessReach[e.ps->list[i]] = 3;   // v15: sprint through pearls
                }
            }
            int pd = std::min(depth, (int)P_KING_PESS2_DEPTH);
            int sp = survival(s.body, L, 1, 1 + pd, std::max(200L, dlim / 4), pessReach.p) - 1;
            if (sp < pd) score -= P_KING_PESS2;
        }
    }
    // portal exits we cannot see into
    {
        int cur = headT;
        for (int d : c.dirs) {
            int v = nb[cur][d];
            if (v < 0) break;
            if (viaPortal[cur][d] && !visible[v]) score -= portalBlind();
            cur = v;
        }
        // parking my head on a portal landing tile: someone may come through into it
        if (portalExitStay() > 0) {
            int h = s.body[0];
            for (int d = 0; d < 4; d++) {
                int16_t e; edgeCode(h, d, e);
                if (e >= 0) { score -= portalExitStay(); break; }
            }
        }
    }
    // effect on nearby allies
    if (!allies.empty()) {
        bool any = false;
        for (auto& a : allies) if (manhattan(a.head, s.body[0]) <= 4) any = true;
        if (any) {
            for (int t : myBody) newMine[t] = 0;
            int nbody = s.body.size();
            for (int i = 0; i < nbody; i++) newMine[s.body[i]] = (int16_t)(L - i + 1);
            for (auto& a : allies) {
                if (manhattan(a.head, s.body[0]) > 4) continue;
                int da = std::min(a.len + 2, P_SURV_DEPTH);
                std::vector<int> ab = {a.head};
                bool aKing = a.id == kingK.id;
                if (aKing) da = std::min(a.len + 2, P_KING_SURV_DEPTH);
                int sa = survival(ab, a.len, 0, da, aKing ? P_KING_DFS_LIMIT / 2 : P_DFS_LIMIT / 3, newMine.p);
                if (sa < da) score -= aKing ? P_W_ALLY_TRAP * 3 : P_W_ALLY_TRAP;
            }
            for (int i = 0; i < nbody; i++) newMine[s.body[i]] = 0;
        }
    }
    // give the king room
    if (!isKingFlag && kingDist && P_KING_SPACE > 0) {
        int dk = kingDist->d[s.body[0]];
        if (dk != UNR && dk <= 2) score -= P_KING_SPACE * (3 - dk);
        // v10: before the endgame pull, keep further out of the king's way (on Schooltime our own swarm
        // boxed kings in until they split themselves to escape)
        if (P_KING_SPACE2 > 0 && rnd < cfg.feedPullStart && dk != UNR && dk <= P_KING_SPACE2_R)
            score -= P_KING_SPACE2 * (P_KING_SPACE2_R + 1 - dk);
    }
    // goal field (precomputed per first move)
    score += P_W_EAT * s.eaten * (growMul < 0 ? growMul : 1.0);
    if (P_KING_DROP_PEN > 0 && s.eatenNearKing > 0) score -= P_KING_DROP_PEN * s.eatenNearKing;   // v15
#ifdef DEBUGLOG
    if (getenv("KDROP_DEBUG") && kingDropOn && s.eaten > 0) fprintf(stderr, "KDROP r%d id%d eaten %d near %d\n", rnd, myId, s.eaten, s.eatenNearKing);
#endif
    score += P_W_GOAL * G[c.dirs[0]];
    // v14: keep apart from our own heads (on the ladder our heads had 4.6 others of ours within 3 tiles on
    // Schooltime, the top team's 1.3; 230-300 of our dragons a game died boxed in by our own bodies)
    if (P_SPREAD_W > 0 && (spreadOn() || (dbMap < 0 && rnd < P_EARLY_SPREAD)) && !isKingFlag && rnd < cfg.feedPullStart) {
        double pen = 0;
        for (auto& a : allies) {
            int d = cheb(a.head, s.body[0]);
            if (d <= P_SPREAD_R) pen += P_SPREAD_R + 1 - d;
        }
        score -= P_SPREAD_W * pen;
    }
    // space (voronoi vs enemies)
    spaceAllyNow = P_KING_SPACE_ALLY && isKingFlag && rnd < cfg.feedPullStart;
    // v14: on the ladder (Trauma) a 2-long enemy walked up to our coiled 34-long king and closed its only
    // way out: the room behind a narrow exit counted as the king's because it got there one move sooner
    spaceMargin = (P_KING_SPACE_MARGIN > 0 && (isKingFlag || L >= P_KING_SPACE_MARGIN_LEN)) ? (int)P_KING_SPACE_MARGIN : 0;
    int space = spaceCount(s.body[0], 1, L + 3);
    spaceAllyNow = false; spaceMargin = 0;
    if (space < L + 3 && !pocketIn) score -= P_W_SPACE * (L + 3 - space) * (1.0 + L / P_SPACE_LEN_SCALE);
    // strike threat on the new head
    score -= riskMult() * threatCost(s.body[0], L);
    // long dragons: enemies seen recently but out of view now may be in sprint reach (they strike
    // from outside the 7x7 view: a 7-long unit reaches 6 tiles)
    if (P_KING_EXPO > 0 && (isKingFlag || L >= P_EXPO_MINLEN)) {
        int h = s.body[0];
        double risk = 0;
        for (auto& g : sightings) {
            int age = rnd - g.round;
            if (age > P_EXPO_AGE || g.len < 3) continue;
            bool vis = false;
            for (auto& e : enemies) if (e.id == g.id) vis = true;
            if (vis) continue;
            int reach = g.len - 1 + age;
            int d = manhattan(g.tile, h);
            if (d <= reach) risk += std::min(1.0, (reach - d + 1) / 3.0) / (1.0 + age / 4.0);
        }
        score -= P_KING_EXPO * L * risk;
    }
    if (c.dirs.size() > 1) score -= P_SPRINT_COST * (c.dirs.size() - 1);
    if (c.dirs[0] == myFacing) score += 0.01;
    c.score = score;
}

// sprint path to an enemy head (kamikaze)
std::vector<int> attackPath(int target, int maxSteps) {
    static TileArr<int16_t> d;
    static TileArr<int8_t> via;
    static TileArr<int> prevT;
    static TileArr<int16_t> myBehind;
    static TileArr<int> touched;
    int nt = 0;
    std::vector<int> path;
    for (int i = 0; i < (int)myBody.size(); i++) myBehind[myBody[i]] = (int16_t)(myLen - 1 - i);
    int qh = 0, qt = 0;
    static TileArr<int> q;
    d[headT] = 0; touched[nt++] = headT; q[qt++] = headT;
    bool found = false;
    while (qh < qt && !found) {
        int u = q[qh++];
        if (d[u] >= maxSteps) continue;
        int j = d[u] + 1;
        for (int dir = 0; dir < 4; dir++) {
            int v = nb[u][dir];
            if (v < 0) continue;
            bool seen = false;
            for (int k = 0; k < nt; k++) if (touched[k] == v) { seen = true; break; }
            if (seen) continue;
            if (v == target) { d[v] = j; via[v] = dir; prevT[v] = u; touched[nt++] = v; found = true; break; }
            if (occId[v] >= 0 && occId[v] != myId) continue;
            if (occId[v] == myId) {
                int freed = j >= 2 ? 2 * j - 3 : 0;
                if (myBehind[v] >= freed) continue;
            }
            d[v] = j; via[v] = dir; prevT[v] = u; touched[nt++] = v; q[qt++] = v;
            if (nt > 400) break;
        }
    }
    if (found) {
        int cur = target;
        while (cur != headT) { path.push_back(via[cur]); cur = prevT[cur]; }
        std::reverse(path.begin(), path.end());
    }
    return path;
}

bool splitOK(int k) {
    if (k < 2 || myLen - k < 2) return false;
    if (unitCount >= UNIT_LIMIT) return false;
    int L = myBody.size();
    if (L != myLen) return false;
    int ch = myBody[L - 1];
    int cneck = myBody[L - 2];
    int freeN = 0;
    for (int d = 0; d < 4; d++) {
        int v = nb[ch][d];
        if (v < 0 || v == cneck) continue;
        if (occId[v] >= 0) continue;
        freeN++;
    }
    if (freeN == 0) return false;
    for (auto& e : enemies) {
        if (manhattan(e.head, headT) <= P_SPLIT_ENEMY_DIST) return false;
        if (manhattan(e.head, ch) <= P_SPLIT_ENEMY_DIST) return false;
    }
    int crowd = 0;
    for (auto& p : vparts) {
        if (p.id == myId) continue;
        int t = ID(p.x, p.y);
        if (cheb(t, headT) <= 2 || cheb(t, ch) <= 2) crowd++;
    }
    if (crowd > P_SPLIT_CROWD) return false;
    {
        int dens = 0;
        for (auto& p : vparts) if (p.id != myId && p.team == myTeam) dens++;
        if (dens > P_SPLIT_DENSITY) return false;
    }
    timeBFS(tmpMap, headT, 0, 30);
    if (tmpMap.n < P_SPLIT_SPACE_MULT * myLen) return false;
    std::vector<int> pb(myBody.begin(), myBody.begin() + (L - k));
    int dep = std::min(L - k + 2, P_SURV_DEPTH);
    int s = survival(pb, L - k, 0, dep, 4000);
    if (s < dep) return false;
    std::vector<int> cb;
    for (int i = L - 1; i >= L - k; i--) cb.push_back(myBody[i]);
    // child's survival (it moves later this round, parent is static)
    for (int i = 0; i < L - k; i++) newMine[myBody[i]] = BIG_FREE;
    int cdep = std::min(k + 2, P_SURV_DEPTH);
    int cs = survival(cb, k, 0, cdep, 4000, newMine.p);
    for (int i = 0; i < L - k; i++) newMine[myBody[i]] = 0;
    if (cs < cdep) return false;
    return true;
}

int splitSize() {
    int k = myLen / 2;
    if (P_CHILD_MAX > 0 && k > P_CHILD_MAX) k = P_CHILD_MAX;
    return k;
}

bool wantSplit() {
    if (myLen < cfg.splitMin) return false;
    if (isolated()) return false;
    if (rnd >= P_SPLIT_END) return false;
    if (rnd >= cfg.growRound) return false;
    if (isKing() && rnd >= cfg.kingSplitUntil) return false;
    if (unitCount >= UNIT_LIMIT) return false;
    if (unitCount >= cfg.maxUnits) return false;
    if (unitCount >= P_GROW_UNITS) return false;   // v16: keep the last slots for escape splits
    if (myLen >= P_KING_LEN && P_KING_NOSPLIT) return false;
    // v9: a long dragon that is not (or no longer) the king stays whole: a second big dragon is insurance
    // for when the king dies and counts for the total-length tiebreak (two ladder games were lost 61-61
    // and 41-41 on total length); shredding it into 2-long units threw that away
    if (P_BIG_NOSPLIT_LEN > 0 && myLen >= P_BIG_NOSPLIT_LEN && rnd >= cfg.kingSplitUntil) return false;
    if (P_AREA_PER_UNIT > 0 && unitCount * P_AREA_PER_UNIT > hf.n) return false;
    return true;
}

// After SPLIT k the child is my last k segments reversed. It cannot see most of a long body, so
// send it the layout over sonar on every ray (from my head, or my tail for the backward ray)
// whose first hit is one of the child's segments.
void prepareBodyMsgs(int k) {
    bodyMsgs.clear();
    int L = myLen;
    if (k < 6) return;
    std::vector<int> cb;
    for (int i = L - 1; i >= L - k; i--) cb.push_back(myBody[i]);
    std::vector<int> dirs;
    for (size_t i = 0; i + 1 < cb.size(); i++) {
        int dd = -1;
        for (int d = 0; d < 4; d++) if (nb[cb[i]][d] == cb[i + 1]) dd = d;
        if (dd < 0) return;
        dirs.push_back(dd);
    }
    static TileArr<bool> inChild;
    for (int t : cb) inChild[t] = true;
    std::vector<int> pb(myBody.begin(), myBody.begin() + (L - k));
    int hitDirs[4], nh = 0;
    for (int d = 0; d < 4; d++) {
        int start = pb[0], rd = d;
        if (pb.size() >= 2 && d == (myFacing + 2) % 4) {
            start = pb.back();
            int d2 = -1;
            for (int e = 0; e < 4; e++) if (nb[pb.back()][e] == pb[pb.size() - 2]) d2 = e;
            if (d2 < 0) continue;
            rd = (d2 + 2) % 4;
        }
        int cur = start; bool hit = false;
        for (int step = 0; step < W + H; step++) {
            int nx = nb[cur][rd];
            if (nx < 0) break;
            cur = nx;
            if (inChild[cur]) { hit = true; break; }
            if (occId[cur] >= 0) break;
        }
        if (hit) hitDirs[nh++] = d;
    }
    for (int t : cb) inChild[t] = false;
    // the child sees roughly its first few segments itself: start the layout where its view ends
    int vis = 0;
    for (int i = 0; i < (int)cb.size() && cheb(cb[i], cb[0]) <= 3; i++) vis++;
    int start = std::max(0, vis - 2);
    for (int j = 0; j < nh && start < (int)dirs.size() && start < 127; j++) {
        int n = std::min(16, (int)dirs.size() - start);
        uint64_t pl = (uint64_t)(start & 127) | ((uint64_t)n << 7) | ((uint64_t)(cb[start] % 61) << 12);
        for (int i = 0; i < n; i++) pl |= (uint64_t)dirs[start + i] << (18 + 2 * i);
        bodyMsgs.push_back({hitDirs[j], gEncode(GT_BODY, pl)});
        start += n;
    }
#ifdef DEBUGLOG
    if (getenv("BODY_DEBUG")) fprintf(stderr, "r%d id%d SPLIT %d rays%d msgs%zu\n", rnd, myId, k, nh, bodyMsgs.size());
#endif
}

// ------------------------------------------------------------------ production zones (known maps)
// Clusters of fast-respawning tiles (Schooltime's (1,20) block, Trophy's centre room, Devil's (1,30)
// middle) are worth a long walk early in the game: the ladder teams rush them from round 0.
// v16 zone hold (known elimination maps): on the ladder the teams that beat us kept 2-3 heads for
// hundreds of rounds in each map's jackpot (Devil's centre column: 85-166 pearls in 200 rounds; Trophy's
// cup) and ate there while our dragons came and went. A share of our short dragons (by id) is given
// "hold" duty: walk to the zone, and once inside, moves that leave it cost P_HOLD_STAY.
struct HoldRect { const char* map; int x0, y0, x1, y1, cap; };   // cap: at most this many of our heads in the zone (0 = no cap)
const HoldRect HOLD_TABLE[] = { P_HOLD_TABLE };
std::vector<char> holdIn; std::vector<int16_t> holdDist; int holdFor = -3, holdCap = 0;
bool buildHold() {
    if (holdFor == dbMap) return !holdIn.empty();
    holdFor = dbMap; holdIn.clear(); holdDist.clear(); holdCap = 0;
    if (dbMap < 0) return false;
    bool any = false;
    std::vector<char> in(N, 0);
    for (const HoldRect& R : HOLD_TABLE) {
        if (!R.map || strcmp(R.map, MAPDB[dbMap].name)) continue;
        if (R.cap > 0) holdCap = holdCap > 0 ? std::max(holdCap, R.cap) : R.cap;
        for (int x = R.x0; x <= R.x1; x++)
            for (int y = R.y0; y <= R.y1; y++) { int t = ID(x, y); if (t >= 0 && t < N) { in[t] = 1; any = true; } }
    }
    if (!any) return false;
    holdIn = in;
    holdDist.assign(N, (int16_t)UNR);
    std::vector<int> q;
    for (int t = 0; t < N; t++) if (holdIn[t]) { holdDist[t] = 0; q.push_back(t); }
    for (size_t qh = 0; qh < q.size(); qh++) {
        int u = q[qh];
        for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && holdDist[v] == UNR) { holdDist[v] = (int16_t)(holdDist[u] + 1); q.push_back(v); } }
    }
    return true;
}
bool holderRole();
struct Zone { int center; double rate; std::vector<int16_t> dist; };
std::vector<Zone> zones;
TileArr<double> zoneRateAt;
int zonesFor = -2;
double meanRate = 0;
void staticDist(int src, std::vector<int16_t>& out);
void buildZones() {
    if (zonesFor == dbMap) return;
    zonesFor = dbMap;
    zones.clear();
    if (dbMap < 0 || (P_ZONE_PULL <= 0 && cfg.zonePull <= 0 && P_OPEN_RUSH <= 0)) return;
    buildPockets();
    static TileArr<double> rate;
    int cnt = 0; double tot = 0;
    for (int t = 0; t < N; t++) {
        rate[t] = gapB[t] > 0 && (!pocketOf[t] || (P_ZONE_POCKETS && cfg.pocketMaxLen <= 0)) ? 2.0 / (gapA[t] + gapB[t]) : 0.0;   // pockets are farmed separately (v11: unless this map farms no pockets: Default's portal boxes)
        if (gapB[t] > 0) { cnt++; tot += rate[t]; }
    }
    meanRate = cnt ? tot / N : 0;
    // rate within a small walking radius of every tile
    static TileArr<int16_t> dd;
    for (int t = 0; t < N; t++) dd[t] = -1;
    std::vector<int> q;
    for (int t = 0; t < N; t++) {
        q.clear(); q.push_back(t); dd[t] = 0;
        double s = 0;
        for (size_t qh = 0; qh < q.size(); qh++) {
            int u = q[qh];
            s += rate[u];
            if (dd[u] >= P_ZONE_RADIUS) continue;
            for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && dd[v] < 0) { dd[v] = dd[u] + 1; q.push_back(v); } }
        }
        for (int u : q) dd[u] = -1;
        zoneRateAt[t] = s;
    }
    // local maxima that clearly beat the average area, at most a few, well apart
    double area = 0;
    { double s = 0; int n = 0; for (int t = 0; t < N; t++) if (nb[t][0] >= 0 || nb[t][1] >= 0 || nb[t][2] >= 0 || nb[t][3] >= 0) { s += zoneRateAt[t]; n++; } area = n ? s / n : 0; }
    std::vector<int> order(N);
    for (int t = 0; t < N; t++) order[t] = t;
    std::sort(order.begin(), order.end(), [](int a, int b) { return zoneRateAt[a] > zoneRateAt[b]; });
    for (int t : order) {
        if ((int)zones.size() >= 8) break;
        if (zoneRateAt[t] < P_ZONE_MIN_RATIO * area || zoneRateAt[t] < P_ZONE_MIN_RATE) break;
        bool far = true;
        for (auto& z : zones) if (z.dist[t] != UNR && z.dist[t] < 10) far = false;
        if (!far) continue;
        Zone z; z.center = t; z.rate = zoneRateAt[t];
        staticDist(t, z.dist);
        zones.push_back(z);
    }
#ifdef DEBUGLOG
    if (getenv("ZONE_DEBUG")) {
        fprintf(stderr, "zones for %s (area %.3f):", MAPDB[dbMap].name, area);
        for (auto& z : zones) fprintf(stderr, " (%d,%d) %.2f", TX(z.center), TY(z.center), z.rate);
        fprintf(stderr, "\n");
    }
#endif
}

// unknown maps: production zones from the tiles seen so far (a tile whose countdowns never exceeded
// a small value respawns fast). Rebuilt every few rounds.
int zonesOnlineRnd = -100;
void buildZonesOnline() {
    if (P_ZONE_ONLINE <= 0) return;
    // rebuilt every turn from this dragon's own knowledge (cheap; nothing cached across dragons)
    zonesFor = -1;
    zonesOnlineRnd = rnd;
    zones.clear();
    for (int t = 0; t < N; t++) zoneRateAt[t] = 0;
    static TileArr<int16_t> dd;
    static bool ddInit = false;
    if (!ddInit) { for (int t = 0; t < N; t++) dd[t] = -1; ddInit = true; }
    std::vector<int> q, touched;
    int nrich = 0;
    for (int t = 0; t < N; t++) {
        const TileK& K = tk[t];
        double g = K.gN > 0 ? (double)K.gSum / K.gN : (K.cdMax > 0 ? 1.2 * K.cdMax : 0.0);
        if (g <= 0 || g > P_ZONE_RICH_GAP) continue;
        nrich++;
        double r = 1.0 / g;
        q.clear(); q.push_back(t); dd[t] = 0;
        for (size_t qh = 0; qh < q.size(); qh++) {
            int u = q[qh];
            zoneRateAt[u] += r;
            if (dd[u] >= P_ZONE_RADIUS) continue;
            for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && dd[v] < 0) { dd[v] = dd[u] + 1; q.push_back(v); } }
        }
        for (int u : q) dd[u] = -1;
    }
    if (nrich < 3) return;
    std::vector<int> order;
    for (int t = 0; t < N; t++) if (zoneRateAt[t] >= P_ZONE_MIN_RATE) order.push_back(t);
    std::sort(order.begin(), order.end(), [](int a, int b) { return zoneRateAt[a] > zoneRateAt[b]; });
    for (int t : order) {
        if ((int)zones.size() >= 4) break;
        bool far = true;
        for (auto& z : zones) if (z.dist[t] != UNR && z.dist[t] < 10) far = false;
        if (!far) continue;
        Zone z; z.center = t; z.rate = zoneRateAt[t];
        staticDist(t, z.dist);
        zones.push_back(z);
    }
#ifdef DEBUGLOG
    if (getenv("ZONE_DEBUG") && !zones.empty()) {
        fprintf(stderr, "r%d id%d online zones (%d rich tiles):", rnd, myId, nrich);
        for (auto& z : zones) fprintf(stderr, " (%d,%d) %.2f", TX(z.center), TY(z.center), z.rate);
        fprintf(stderr, "\n");
    }
#endif
}

// map-only distances from src (bodies ignored): long-range guidance beyond the planner's horizon
void staticDist(int src, std::vector<int16_t>& out) {
    out.assign(N, (int16_t)UNR);
    std::vector<int> q = {src};
    out[src] = 0;
    for (size_t qh = 0; qh < q.size(); qh++) {
        int u = q[qh];
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || out[v] != UNR) continue;
            out[v] = (int16_t)(out[u] + 1); q.push_back(v);
        }
    }
}
// add `w` to the goal of the first moves that lead (around my own body, via the planner's head
// field) to the reachable tile closest to the target of the static distance map `kd`
void pullAlong(const std::vector<int16_t>& kd, double w) {
    int best = -1, bk = UNR, bh = UNR;
    for (int i = 0; i < hf.n; i++) {
        int t = hf.list[i];
        if (kd[t] < bk || (kd[t] == bk && hf.d[t] < bh)) { bk = kd[t]; bh = hf.d[t]; best = t; }
    }
    if (best < 0 || bk >= kd[headT]) return;
    for (int d = 0; d < 4; d++) if (hf.bits[best] >> d & 1) G[d] += w;
}
// v5: the endgame meeting tile: the most open tile (most tiles within 3 steps) that I have seen,
// within P_RV_SEARCH steps of `center`
int chooseRV(int center) {
    static TileArr<int16_t> d1, d2;
    std::vector<int> q1 = {center}, q2;
    d1[center] = 0;
    int best = center, bestOpen = -1, bestD = 0;
    for (size_t i = 0; i < q1.size(); i++) {
        int u = q1[i];
        if (tk[u].pR >= 0 || u == center) {
            // openness of u
            q2.clear(); q2.push_back(u); d2[u] = 1;
            for (size_t j = 0; j < q2.size(); j++) {
                int w = q2[j];
                if (d2[w] > 3) continue;
                for (int dd = 0; dd < 4; dd++) { int v = nb[w][dd]; if (v >= 0 && !d2[v]) { d2[v] = d2[w] + 1; q2.push_back(v); } }
            }
            int open = (int)q2.size();
            for (int w : q2) d2[w] = 0;
            if (open > bestOpen || (open == bestOpen && d1[u] < bestD)) { bestOpen = open; best = u; bestD = d1[u]; }
        }
        if (d1[u] >= P_RV_SEARCH) continue;
        for (int dd = 0; dd < 4; dd++) { int v = nb[u][dd]; if (v >= 0 && v != center && !d1[v]) { d1[v] = d1[u] + 1; q1.push_back(v); } }
    }
    for (int u : q1) d1[u] = 0;
    return best;
}
// v6: tiles within [dmin, dmax] steps of c through the map (walls and portals; bodies ignored).
// Manhattan distance is wrong in mazes and room maps: a donor 3 tiles away behind a wall is not "near".
void pathRing(int c, int dmin, int dmax, std::vector<int>& out) {
    static TileArr<int16_t> dd;
    out.clear();
    std::vector<int> q = {c};
    dd[c] = 1;
    for (size_t i = 0; i < q.size(); i++) {
        int u = q[i];
        if (dd[u] - 1 >= dmin) out.push_back(u);
        if (dd[u] - 1 >= dmax) continue;
        for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && !dd[v]) { dd[v] = dd[u] + 1; q.push_back(v); } }
    }
    for (int u : q) dd[u] = 0;
}
int pathDist(int a, int b, int maxd) {
    if (a == b) return 0;
    static std::vector<int> ring;
    static TileArr<int16_t> dd;
    std::vector<int> q = {a};
    dd[a] = 1;
    int res = 99;
    for (size_t i = 0; i < q.size() && res == 99; i++) {
        int u = q[i];
        if (dd[u] - 1 >= maxd) continue;
        for (int d = 0; d < 4; d++) {
            int v = nb[u][d];
            if (v < 0 || dd[v]) continue;
            dd[v] = dd[u] + 1; q.push_back(v);
            if (v == b) { res = dd[v] - 1; break; }
        }
    }
    for (int u : q) dd[u] = 0;
    return res;
}
// v12: feed only the king (never "the biggest dragon at hand" when the king is far): P_FEED_KING_ONLY
// everywhere, P_FKO_KNOWN on the known maps. On Schooltime the feeders at the unit cap fed whichever longer
// ally was near and the king stayed short until round 400 (v10 README point 5)
inline bool feedKingOnly() { return P_FEED_KING_ONLY || (P_FKO_KNOWN && dbMap >= 0) || (P_PORTALS_FKO && onPortalsMap()); }
// v14: an enemy head within P_KING_GUARD_R of our (fresh) king, shorter than it, is a target for our short dragons
bool inKingField(int t);
bool swarmSeen();
bool kingGuardTarget(const Threat& e) {
    if (P_KING_GUARD <= 0 || iAmKing || myLen > P_KING_GUARD_MAXLEN || myLen < 2) return false;
    if (P_KING_GUARD == 1 && dbMap < 0) return false;
    // v15: on Trauma any enemy head inside our home field is a target (the king lives there)
    if (P_HOME_GUARD && roleMaps() && dbMap >= 0 && farmerRole != 2 && inKingField(e.head) && swarmSeen()) return true;
    if (kingK.id < 0 || kingK.round < rnd - 5 || kingK.len < P_KING_GUARD_MINKING) return false;
    return e.len < kingK.len && manhattan(e.head, kingK.tile) <= P_KING_GUARD_R;
}
// v12: where the local-numbers rules (P_ATTACK_LOCAL, P_OUTNUM_P, P_REINFORCE) apply
inline bool localRulesOn() {
    if (P_ATTACK_LOCAL_WHERE == 2) return true;
    if (dbMap < 0) return P_LOCAL_UNKNOWN && dbWallsOf < 0 && N > 0 && N < P_BIG_MAP_TILES;   // v17: small unknown maps play by the known elimination maps' fight rules
    if (P_ATTACK_LOCAL_WHERE == 3) return cfg.kingSplitUntil >= 9999 && cfg.splitMin < 999;   // the elimination maps
    return true;
}
inline bool rvActive() { return rvMode() && rvTile >= 0 && rnd >= cfg.feedPullStart && cfg.feedStart >= 300; }
bool feedingNow = false;
bool heirNow = false;   // v16: this courier keeps its length (Trauma heir)
const char* const SECTOR_MAPS[] = { P_SECTOR_MAPS };
bool sectorMapOK() {
    if (dbMap < 0) return false;
    for (const char* m : SECTOR_MAPS) if (!strcmp(m, "*") || !strcmp(m, MAPDB[dbMap].name)) return true;
    return false;
}
bool holderRole() {
    if (!P_HOLD || dbMap < 0 || isKingFlag || iAmKing || feedingNow || rnd < P_HOLD_START || rnd >= P_HOLD_UNTIL || myLen > P_HOLD_MAXLEN) return false;
    if ((mix64((uint64_t)myId * 104729u + 29) % 1000) >= (uint64_t)(P_HOLD_FRAC * 1000)) return false;
    if (!buildHold()) return false;
    if (holdCap > 0) {   // enough of us in the zone already (as far as I can see): no duty
        int n = 0;
        for (auto& a : allies) if (holdIn[a.head]) n++;
        if (n >= holdCap) return false;
    }
    return true;
}
bool rushingNow = false;   // v16: this dragon is on its opening rush and has not arrived
// v16 sector spread (known maps): on the ladder the top team's heads were spread evenly over every map
// (Schooltime: 40-150 head samples in each 10x5 cell) while 65% of ours sat in the bottom-centre block and
// the rest of the map was theirs. Each of our dragons gets a home sector (by id, weighted by the sector's
// pearl production) and a pull toward it when it is elsewhere.
std::vector<int> secRep; std::vector<double> secW; int secFor = -3;
TileArr<int16_t> secOf;
void buildSectors() {
    int key = dbMap;
    if (secFor == key) return;
    secFor = key; secRep.clear(); secW.clear();
    if (dbMap < 0) return;
    int sw = std::max(4, (int)P_SECTOR_SIZE), nx = (W + sw - 1) / sw, ny = (H + sw - 1) / sw;
    std::vector<double> w(nx * ny, 0.0), bestR(nx * ny, -1.0);
    std::vector<int> rep(nx * ny, -1);
    for (int t = 0; t < N; t++) {
        int sc = (TY(t) / sw) * nx + TX(t) / sw;
        secOf[t] = (int16_t)sc;
        if (gapB[t] > 0) {
            double rt = 2.0 / (gapA[t] + gapB[t]);
            w[sc] += rt;
            if (rt > bestR[sc]) { bestR[sc] = rt; rep[sc] = t; }
        }
    }
    for (int sc = 0; sc < nx * ny; sc++) { secRep.push_back(rep[sc]); secW.push_back(rep[sc] >= 0 ? w[sc] : 0.0); }
}
int homeSector() {
    buildSectors();
    double tot = 0;
    for (double x : secW) tot += x;
    if (tot <= 0) return -1;
    double u = (double)(mix64((uint64_t)myId * 6364136223846793005ULL + 1442695040888963407ULL) % 1000000) / 1e6 * tot;
    for (size_t i = 0; i < secW.size(); i++) { if (u < secW[i]) return (int)i; u -= secW[i]; }
    return (int)secW.size() - 1;
}
int huntTile = -1;   // v5: target of an endgame hunter (-1 = none)
int kingHomeTile();
// v7 Trauma plan 3: the king's field = what it reaches from its start without portals; its anchor is the
// field tile closest (through the portals) to our farm pocket, where the couriers come out
std::vector<char> kingField, teamField;   // teamField: kingField + the region of our pocket
int kingAnchor = -1, kingFieldFor = -2;
void buildKingField() {
    int key = dbMap * 2 + mySide();
    if (kingFieldFor == key) return;
    kingFieldFor = key; kingField.clear(); teamField.clear(); kingAnchor = -1;
    int h = kingHomeTile();
    if (h < 0) return;
    kingField.assign(N, 0);
    std::vector<int> q = {h}; kingField[h] = 1;
    for (size_t i = 0; i < q.size(); i++)
        for (int d = 0; d < 4; d++) { int v = nb[q[i]][d]; if (v >= 0 && !viaPortal[q[i]][d] && !kingField[v]) { kingField[v] = 1; q.push_back(v); } }
    int bd = 1 << 30;
    teamField = kingField;
    for (auto& P : pockets) {
        if (!P.ours) continue;
        for (int t : q) if (P.dist[t] != UNR && P.dist[t] < bd) { bd = P.dist[t]; kingAnchor = t; }
        std::vector<int> r = {P.junction};
        if (!teamField[P.junction]) teamField[P.junction] = 1; else continue;
        for (size_t i = 0; i < r.size(); i++)
            for (int d = 0; d < 4; d++) { int v = nb[r[i]][d]; if (v >= 0 && !viaPortal[r[i]][d] && !teamField[v]) { teamField[v] = 1; r.push_back(v); } }
    }
}
bool inKingField(int t) {
    buildKingField();
    return kingField.size() == (size_t)N && kingField[t];
}
// v15 Trauma gate: our home field is reached only through two portals. A 2- or 3-long courier circles the
// 2x2 block at the landing tile of the one our couriers do not use: an enemy stepping through lands on its
// body (and dies) three times in four; otherwise it lands next to the gatekeeper's head.
int gateFor = -3, gateLand = -1;
int gateBlk[4] = {-1, -1, -1, -1};   // in cycle order
void buildGate() {
    int key = dbMap * 2 + mySide();
    if (gateFor == key) return;
    gateFor = key; gateLand = -1; for (int& b : gateBlk) b = -1;
    buildKingField();
    if (kingField.size() != (size_t)N || kingAnchor < 0) return;
    // landing tiles: field tiles with a portal edge to outside the field
    std::vector<int> lands;
    for (int t = 0; t < N; t++) {
        if (!kingField[t]) continue;
        for (int d = 0; d < 4; d++) { int v = nb[t][d]; if (v >= 0 && viaPortal[t][d] && !kingField[v]) { lands.push_back(t); break; } }
    }
    if (lands.size() < 2) return;
    // couriers arrive at the landing nearest the anchor; the gate goes on the farthest
    static TileArr<int16_t> dd;
    for (int t = 0; t < N; t++) dd[t] = -1;
    std::vector<int> q = {kingAnchor}; dd[kingAnchor] = 0;
    for (size_t i = 0; i < q.size(); i++)
        for (int d = 0; d < 4; d++) { int v = nb[q[i]][d]; if (v >= 0 && !viaPortal[q[i]][d] && kingField[v] && dd[v] < 0) { dd[v] = dd[q[i]] + 1; q.push_back(v); } }
    int best = -1, bdist = -1;
    if (P_GATE_DOOR == 1) { bdist = 1 << 30; for (int t : lands) if (dd[t] >= 0 && dd[t] < bdist) { bdist = dd[t]; best = t; } if (best < 0) return; }   // v15: the couriers' door (on the ladder the invaders' one)
    else {
        for (int t : lands) if (dd[t] > bdist) { bdist = dd[t]; best = t; }
        if (best < 0 || bdist < P_GATE_MINDIST) return;
    }
    // a 2x2 block of field tiles containing it, joined without portals
    for (int oy = -1; oy <= 0 && gateLand < 0; oy++)
        for (int ox = -1; ox <= 0 && gateLand < 0; ox++) {
            int x0 = TX(best) + ox, y0 = TY(best) + oy;
            int a = ID(x0, y0), b = ID(x0 + 1, y0), c = ID(x0 + 1, y0 + 1), e = ID(x0, y0 + 1);
            int cyc[4] = {a, b, c, e};
            bool ok = true;
            for (int k = 0; k < 4 && ok; k++) {
                int u = cyc[k], v = cyc[(k + 1) % 4];
                if (!kingField[u]) { ok = false; break; }
                bool link = false;
                for (int d = 0; d < 4; d++) if (nb[u][d] == v && !viaPortal[u][d]) link = true;
                if (!link) ok = false;
            }
            if (ok) { gateLand = best; for (int k = 0; k < 4; k++) gateBlk[k] = cyc[k]; }
        }
}
inline int gateIdx(int t) { for (int k = 0; k < 4; k++) if (gateBlk[k] == t) return k; return -1; }
// v15: the other team plays as a swarm (at least P_GATE_SWARM of its dragons seen in the last 60 rounds)
bool swarmSeen() {
    if (P_GATE_SWARM <= 0) return true;
    static int memoRound = -1; static bool memo = false;
    if (memoRound == rnd) return memo;
    memoRound = rnd;
    int cnt = 0;
    for (auto& g : sightings) if (g.round >= rnd - 60) cnt++;
    memo = cnt >= P_GATE_SWARM;
    return memo;
}
// v15: the king keeps to its home field once the other team is seen as a swarm (P_FENCE_SWARM of its dragons
// in the last 60 rounds): locally the unfenced king walked out through the couriers' portal into the invaders
bool kingFenceOn() {
    if (P_KING_FENCE) return true;
    if (P_FENCE_SWARM <= 0) return false;
    static bool latched = false;
    if (latched) return true;
    int cnt = 0;
    for (auto& g : sightings) if (g.round >= rnd - 60) cnt++;
    if (cnt >= P_FENCE_SWARM && rnd >= P_FENCE_START) latched = true;
    return latched;
}
// v15 king gate (Trauma plan 3): once enemy dragons have been seen inside our home field, the king circles
// a rectangle of myLen + 1 or + 2 tiles that runs through the couriers' landing tile, so that an enemy coming
// through that portal lands on the king's body (on the ladder 17 of the 18 invaders came in that way and
// 13 of their heads were inside at once when our 38-long king was struck).
std::vector<int> kgLoop; int kgLoopLen = -1, kgLoopFor = -3;
TileArr<int16_t> kgIdx;   // 1 + position on the loop (0 = off it)
int kgLand = -1, kgLandFor = -3;
int kgLanding() {
    int key = dbMap * 2 + mySide();
    if (kgLandFor == key) return kgLand;
    kgLandFor = key; kgLand = -1;
    buildKingField();
    if (kingField.size() != (size_t)N || kingAnchor < 0) return -1;
    static TileArr<int16_t> dd;
    for (int t = 0; t < N; t++) dd[t] = -1;
    std::vector<int> q = {kingAnchor}; dd[kingAnchor] = 0;
    for (size_t i = 0; i < q.size(); i++)
        for (int d = 0; d < 4; d++) { int v = nb[q[i]][d]; if (v >= 0 && !viaPortal[q[i]][d] && kingField[v] && dd[v] < 0) { dd[v] = dd[q[i]] + 1; q.push_back(v); } }
    int bd = 1 << 30;
    for (int t : q) {
        bool land = false;
        for (int d = 0; d < 4; d++) { int v = nb[t][d]; if (v >= 0 && viaPortal[t][d] && !kingField[v]) land = true; }
        if (land && dd[t] < bd) { bd = dd[t]; kgLand = t; }
    }
    return kgLand;
}
inline bool plainStep(int u, int v) {
    for (int d = 0; d < 4; d++) if (nb[u][d] == v && !viaPortal[u][d]) return true;
    return false;
}
void buildKgLoop(int L) {
    int key = dbMap * 2 + mySide();
    if (kgLoopFor == key && kgLoopLen == L) return;
    kgLoopFor = key; kgLoopLen = L;
    for (int t : kgLoop) kgIdx[t] = 0;
    kgLoop.clear();
    int land = kgLanding();
    if (land < 0) return;
    int lx = TX(land), ly = TY(land);
    std::vector<int> cyc;
    for (int P = L + 1; P <= L + 2 && kgLoop.empty(); P++) {
        if (P % 2) continue;
        int s2 = (P + 4) / 2;   // w + h
        for (int w = 2; w < s2 - 1 && kgLoop.empty(); w++) {
            int h = s2 - w;
            if (w > W || h > H) continue;
            for (int x0 = lx - w + 1; x0 <= lx && kgLoop.empty(); x0++)
                for (int y0 = ly - h + 1; y0 <= ly && kgLoop.empty(); y0++) {
                    int x1 = x0 + w - 1, y1 = y0 + h - 1;
                    bool onP = (lx == x0 || lx == x1 || ly == y0 || ly == y1);
                    if (!onP) continue;
                    cyc.clear();
                    for (int x = x0; x <= x1; x++) cyc.push_back(ID(x, y0));
                    for (int y = y0 + 1; y <= y1; y++) cyc.push_back(ID(x1, y));
                    for (int x = x1 - 1; x >= x0; x--) cyc.push_back(ID(x, y1));
                    for (int y = y1 - 1; y > y0; y--) cyc.push_back(ID(x0, y));
                    bool ok = (int)cyc.size() == P;
                    for (size_t k = 0; k < cyc.size() && ok; k++) {
                        if (!kingField[cyc[k]]) ok = false;
                        else if (!plainStep(cyc[k], cyc[(k + 1) % cyc.size()])) ok = false;
                    }
                    if (ok) kgLoop = cyc;
                }
        }
    }
    for (size_t k = 0; k < kgLoop.size(); k++) kgIdx[kgLoop[k]] = (int16_t)(k + 1);
}
bool kgActive() {
    if (!P_KING_GATE || !roleMaps() || dbMap < 0 || !isKingFlag || rnd < P_KG_START || rnd >= P_KG_UNTIL) return false;
    static bool latched = false;
    if (!latched) {
        if (P_KING_GATE == 2) latched = true;
        else {
            buildKingField();
            if (kingField.size() == (size_t)N)
                for (auto& g : sightings) if (g.round >= rnd - 100 && kingField[g.tile]) latched = true;
        }
    }
    if (!latched) return false;
    buildKgLoop(myLen);
    return !kgLoop.empty();
}
bool gateEligible() {
    if (!P_TRAUMA_GATE || !roleMaps() || dbMap < 0 || farmerRole != 0 || isKingFlag || rnd < P_GATE_START) return false;
    if (!swarmSeen()) return false;
    if (myLen < 2 || myLen > 3 || (int)myBody.size() != myLen) return false;
    buildGate();
    return gateLand >= 0;
}
bool inGate() {   // my whole body on the block
    if (gateLand < 0) return false;
    for (int t : myBody) if (gateIdx(t) < 0) return false;
    return true;
}
int kingHomeTile() {   // richest starting position of my team (where the king-designate lives)
    if (dbMap < 0) return -1;
    const MapDef& M = MAPDB[dbMap];
    int ti = mySide(), best = -1; double bv = -1;
    int p = 0;
    for (int i = 0; i < M.ndr; i++) {
        int team = M.drg[p], n = M.drg[p + 1], h = M.drg[p + 2];
        if (team == ti) {
            double v = 0;
            for (int t = 0; t < N; t++) if (gapB[t] > 0 && manhattan(t, h) <= 8) v += 2.0 / (gapA[t] + gapB[t]);
            if (v > bv) { bv = v; best = h; }
        }
        p += 2 + n;
    }
    return best;
}

// a farmer's courier split only needs both parts to survive (the maze fails the usual space tests)
bool courierSplitOK(int k) {
    int L = myBody.size();
    if (L != myLen || k < 2 || myLen - k < 2 || unitCount >= UNIT_LIMIT) return false;
    for (auto& e : enemies) if (manhattan(e.head, headT) <= P_SPLIT_ENEMY_DIST || manhattan(e.head, myBody[L - 1]) <= P_SPLIT_ENEMY_DIST) return false;
    std::vector<int> pb(myBody.begin(), myBody.begin() + (L - k));
    int dep = std::min(L - k + 2, P_SURV_DEPTH);
    if (survival(pb, L - k, 0, dep, 4000) < dep) return false;
    std::vector<int> cb;
    for (int i = L - 1; i >= L - k; i--) cb.push_back(myBody[i]);
    for (int i = 0; i < L - k; i++) newMine[myBody[i]] = BIG_FREE;
    int cdep = std::min(k + 2, P_SURV_DEPTH);
    int cs = survival(cb, k, 0, cdep, 4000, newMine.p);
    for (int i = 0; i < L - k; i++) newMine[myBody[i]] = 0;
    return cs >= cdep;
}

inline bool lateKingPhase() { return P_LATE_KING > 0 && cfg.kingSplitUntil >= 9999 && rnd < cfg.feedPullStart - P_LATE_KING; }

void detectSide() {
    if (!P_SIDE_DETECT || sideIdx >= 0 || spawnHead < 0) return;
    int hm = dbMap >= 0 ? dbMap : dbWallsOf;
    if (hm < 0) return;
    const MapDef& M = MAPDB[hm];
    // any dragon I see that started the game (ids follow the map file's order, which the ladder keeps
    // even when it swaps the teams): its team in the file and whether it is ours tell our side
    for (auto& vp : vparts) {
        if (vp.id < 0 || vp.id >= M.ndr) continue;
        int q = 0;
        for (int i = 0; i < vp.id; i++) q += 2 + M.drg[q + 1];
        int F = M.drg[q];
        sideIdx = vp.team == myTeam ? F : 1 - F;
#ifdef DEBUGLOG
        if (getenv("SIDE_DEBUG")) fprintf(stderr, "SIDE r%d id%d team%c saw id%d team%c fileTeam%d -> side %d\n", rnd, myId, myTeam, vp.id, vp.team, F, sideIdx);
#endif
        return;
    }
    int p = 0, best = -1, bd = 1 << 30;
    for (int i = 0; i < M.ndr; i++) {
        int team = M.drg[p], h = M.drg[p + 2];
        if (firstRound == 0 && h == spawnHead) { sideIdx = team; return; }   // an original dragon: exact
        int d = manhattan(h, spawnHead);
        if (d < bd) { bd = d; best = team; }
        p += 2 + M.drg[p + 1];
    }
    if (firstRound == 0 && best >= 0) sideIdx = best;   // a start the map file lacks (Dilemma with 10 dragons): the nearest start
    // (a child waits for its parent's map message, which carries the side)
#ifdef DEBUGLOG
    if (getenv("SIDE_DEBUG")) fprintf(stderr, "SIDE r%d id%d team%c first%d spawn(%d,%d) -> nearest side %d (dist %d)\n", rnd, myId, myTeam, firstRound, TX(spawnHead), TY(spawnHead), sideIdx, bd);
#endif
}
void decide() {
    if (turnCount == 0) spawnHead = headT;
    detectSide();
    probeHit = -1;
    if (P_PORTAL_PROBE && probeDir >= 0 && probeRound == rnd - 1 && probeFrom == headT) {
        int sum = echoes[0] + echoes[1] + echoes[2] + echoes[3] + echoes[4];
        if (sum == 1) probeHit = echoes[0] ? 0 : 1;
#ifdef DEBUGLOG
        if (getenv("PROBE_DBG")) fprintf(stderr, "PROBE r%d id%d at(%d,%d) dir%c echoes %d %d %d %d %d hit=%d\n", rnd, myId, TX(headT), TY(headT), DCH[probeDir], echoes[0], echoes[1], echoes[2], echoes[3], echoes[4], probeHit);
#endif
    }
    if (P_MIRROR_FIX == 1 || P_ATTEMPT_MEMORY) buildHome();   // v7: the mirror fix and attempt memory need our side of the map
    enemies.clear(); allies.clear();
    for (int t = 0; t < N; t++) { distEnemy[t] = UNR; distAlly[t] = UNR; pessExtra[t] = 0; pessAlly[t] = 0; }
    int ne = 0, na = 0;
    bool psAny = false;   // v15: a visible pearl (else sprints reach no farther than length - 1)
    for (int k = 0; k < 49 && !psAny; k++) if (vtile[k].pearl) psAny = true;
    for (auto& D : others) {
        if (D.head < 0) continue;
        Threat T; T.id = D.id; T.head = D.head; T.neck = D.neck; T.len = D.lenEst(); T.lenSure = D.lenKnown > 0;
        if (D.enemy) {
            if (ne >= 24) continue;
            T.dist = poolMap(threatPool, ne++);
            T.reach = std::max(1, T.len - 1);
            if (!T.lenSure) T.reach = std::max(T.reach, P_UNSEEN_REACH);
            otherBFS(*T.dist, T.head, std::max(T.reach, P_COMP_RADIUS));
            if (psOn(3) && psAny && manhattan(T.head, headT) <= P_PSPRINT_MAX + 3) {
                T.ps = poolMap(psPool, ne - 1);
                psReach(*T.ps, T.head, T.len, P_PSPRINT_MAX);
            }
            for (int i = 0; i < T.dist->n; i++) { int t = T.dist->list[i]; distEnemy[t] = std::min(distEnemy[t], T.dist->d[t]); }
            for (int d = 0; d < 4; d++) { int w = nb[T.head][d]; if (w >= 0 && w != T.neck) pessExtra[w] = BIG_FREE; }
            enemies.push_back(T);
            noteSighting(T.head, rnd, T.len, T.id);
        } else {
            if (na >= 24) continue;
            T.dist = poolMap(allyPool, na++);
            T.reach = 0;
            otherBFS(*T.dist, T.head, P_COMP_RADIUS);
            for (int i = 0; i < T.dist->n; i++) { int t = T.dist->list[i]; distAlly[t] = std::min(distAlly[t], T.dist->d[t]); }
            allies.push_back(T);
        }
    }
    PROF("threats");
    // forced-move prediction: a head with exactly one legal move will take it
    forcedTiles.clear();
    for (auto* grp : {&enemies, &allies})
        for (auto& T : *grp) {
            int legal = 0, only = -1;
            for (int d = 0; d < 4; d++) {
                int v = nb[T.head][d];
                if (v < 0 || v == T.neck) continue;
                if (freeOther[v] > 1 || freeMine[v] > 2) continue;
                legal++; only = v;
            }
            if (legal == 1) {
                forcedTiles.push_back(only);
                freeOther[only] = (int16_t)std::max<int>(freeOther[only], std::min(BIG_FREE, T.len + 2));
            }
        }
    if (P_OUTNUM_P > 0 && localRulesOn())
        for (auto& e : enemies) {
            int ne = 0, na = 1;
            for (auto& o : enemies) if (manhattan(o.head, e.head) <= P_OUTNUM_R) ne++;
            for (auto& o : allies) if (manhattan(o.head, e.head) <= P_OUTNUM_R) na++;
            e.backed = ne > na;
        }
    for (auto& T : allies)
        for (int d = 0; d < 4; d++) { int w = nb[T.head][d]; if (w >= 0 && w != T.neck) pessAlly[w] = BIG_FREE; }
    for (int t = 0; t < N; t++) if (pessExtra[t] > pessAlly[t]) pessAlly[t] = pessExtra[t];

    for (auto& D : others) if (D.enemy && D.head >= 0) lastContact = rnd;
    if (rnd == 0 && firstRound == 0)
        for (auto& D : others) if (!D.enemy && D.id < myId) kingVeto = true;
    // ---- king bookkeeping: the king is the longest dragon of the team (ties: lowest id)
    if (kingK.id >= 0 && kingK.id != myId && kingK.round < rnd - kingFreshN()) kingK = {-1, -1, 0, -1};
    kingDist = nullptr;
    for (auto& T : allies) {
        if (T.id == kingK.id) {
            kingDist = T.dist;
            kingK = {T.head, rnd, T.lenSure ? T.len : std::max(T.len, kingK.len), T.id};
        } else if (!roleMaps() && !lateKingPhase() && T.lenSure && T.len >= P_KING_CLAIM_LEN &&
                   (kingK.id < 0 || T.len > kingK.len || (T.len == kingK.len && T.id < kingK.id))) {
            kingDist = T.dist;
            kingK = {T.head, rnd, T.len, T.id};
        }
    }
    buildPockets();
    decideRole();
    // v7: no king heard of for a while (it died): the longest courier around takes the crown
    if (P_KING_REPLACE && roleMaps() && farmerRole == 0 && kingK.id < 0 && turnCount > 5 && myLen >= P_KING_CLAIM_LEN) {
        bool longest = true;
        for (auto& T : allies) if (T.len > myLen || (T.len == myLen && T.id < myId)) longest = false;
        if (longest) farmerRole = 2;
    }
    // roles are fixed there; a second 'king' (a rescue remnant) that hears of a longer one steps down
    if (roleMaps() && farmerRole == 2 && kingK.id >= 0 && kingK.id != myId && kingK.round >= rnd - 20 && kingK.len > myLen)
        farmerRole = 0;
    if (roleMaps() && farmerRole >= 0) iAmKing = P_KING == 1 && farmerRole == 2;
    else if (lateKingPhase()) iAmKing = false;   // pure swarm until the endgame
    else iAmKing = P_KING == 1 && myLen >= P_KING_CLAIM_LEN && farmerRole != 1 &&
              (kingK.id < 0 || kingK.id == myId || myLen > kingK.len || (myLen == kingK.len && myId < kingK.id));
    if (iAmKing) { kingK = {headT, rnd, myLen, myId}; kingDist = nullptr; }
    else if (kingK.id == myId) kingK = {-1, -1, 0, -1};

    // ---- v5: our home = spawn tile of our lowest-id original dragon (known maps: from the map DB)
    if (rvMode() == 2) {
        if (turnCount == 0 && firstRound == 0 && (homeId < 0 || myId < homeId)) { homeTile = headT; homeId = myId; }
        if (dbMap >= 0) {
            const MapDef& M = MAPDB[dbMap];
            int ti = mySide(), p = 0;
            for (int i = 0; i < M.ndr; i++) {
                if (M.drg[p] == ti) { homeTile = M.drg[p + 2]; homeId = i; break; }
                p += 2 + M.drg[p + 1];
            }
        }
        if (homeTile >= 0) { rvTile = homeTile; rvRound = 0; }
    }
    // ---- v5: choose the team's meeting tile (the king does it first; the earliest choice spreads over sonar)
    if (rvMode() == 1 && rvTile < 0 && cfg.feedStart >= 300 &&
        ((iAmKing && rnd >= cfg.feedPullStart - P_RV_LEAD) || rnd >= cfg.feedPullStart)) {
        rvTile = chooseRV(headT); rvRound = rnd;
    }
    // ---- feeding the king: in the endgame, or when the unit cap blocks further splitting
    for (int t : feedTiles) feedVal[t] = 0;
    feedTiles.clear();
    bigAllies.clear();
    // ---- v5 endgame hunters: some small dragons go after the enemy's longest dragon (a head-on
    // kills both, whatever their lengths; the ladder's round-500 games are decided by the longest one)
    huntTile = -1; loneHunt = false;
    // v11: the enemy has merged into a lone king (on the ladder the top team was one 40-54 dragon from
    // round 410 of an Autarky game while we had 20 units 5-12 tiles away, which fed our own king
    // instead): every short non-king dragon hunts it; a head-on kills both, whatever their lengths
    if (P_LONE_HUNT > 0 && (P_LONE_HUNT == 2 || dbMap >= 0) && rnd >= P_LONE_START && !iAmKing && myLen >= 2 && myLen <= P_LONE_MAXLEN && unitCount >= P_LONE_MINUNITS) {
        int cnt = 0; const Sighting* tg = nullptr;
        for (auto& g : sightings)
            if (g.round >= rnd - P_LONE_WINDOW) {
                cnt++;
                if (!tg || g.len > tg->len || (g.len == tg->len && g.round > tg->round)) tg = &g;
            }
        if (cnt > 0 && cnt <= P_LONE_MAX && tg && tg->len >= P_LONE_MINLEN && tg->len >= myLen + 3) {
            huntTile = tg->tile;
            loneHunt = true;
#ifdef DEBUGLOG
            if (getenv("LONE_DEBUG")) fprintf(stderr, "LONE r%d id%d len%d -> target id%d len%d at (%d,%d) seen r%d, %d recent\n", rnd, myId, myLen, tg->id, tg->len, TX(tg->tile), TY(tg->tile), tg->round, cnt);
#endif
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    if (std::abs(dx) + std::abs(dy) > 2) continue;
                    int w = ID(TX(tg->tile) + dx, TY(tg->tile) + dy);
                    feedVal[w] += P_E2_HUNT_ATTRACT * 0.3; feedTiles.push_back(w);
                }
        }
    }
    if (huntTile < 0 && P_E2_HUNT > 0 && rnd >= P_E2_HUNT_START && !iAmKing && myLen >= P_E2_HUNT_MINLEN && myLen <= P_E2_HUNT_MAXLEN &&
        (mix64((uint64_t)myId * 104729u + 31) % 1000) < (uint64_t)(P_E2_HUNT_FRAC * 1000)) {
        const Sighting* tg = nullptr;
        for (auto& g : sightings)
            if (g.round >= rnd - P_E2_HUNT_AGE && g.len >= P_E2_HUNT_TGT && g.len >= myLen + 5 && (!tg || g.len > tg->len || (g.len == tg->len && g.round > tg->round)))
                tg = &g;
        int ourBest = std::max(myLen, kingK.id >= 0 ? kingK.len : 0);
        if (tg && tg->len + P_E2_HUNT_MARGIN >= ourBest) {
            huntTile = tg->tile;
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    if (std::abs(dx) + std::abs(dy) > 2) continue;
                    int w = ID(TX(tg->tile) + dx, TY(tg->tile) + dy);
                    feedVal[w] += P_E2_HUNT_ATTRACT * 0.3; feedTiles.push_back(w);
                }
        }
    }

    // v7: an enemy dragon this long means the game will be decided by the longest dragon: merge earlier
    for (auto& g : sightings) bigEnemySeen = std::max(bigEnemySeen, g.len);
    if (P_E7_BIG_LEN > 0 && bigEnemySeen >= P_E7_BIG_LEN && cfg.feedStart >= 300 && cfg.splitMin < 999) {
        if (cfg.feedStart > P_E7_BIG_FEED) cfg.feedStart = P_E7_BIG_FEED;
        if (cfg.feedPullStart > P_E7_BIG_PULL) cfg.feedPullStart = P_E7_BIG_PULL;
        if (cfg.growRound > P_E7_BIG_FEED) cfg.growRound = P_E7_BIG_FEED;
    }
    // v17: unknown maps without a king (the small-map plan): once an enemy this long has been seen, the other
    // team is building a dragon for the round-500 count, so our longest stops splitting too (from P_DYN_KING_FROM)
    if (P_DYN_KING && dbMap < 0 && dbWallsOf < 0 && bigEnemySeen >= P_DYN_KING_LEN && cfg.splitMin < 999)
        cfg.kingSplitUntil = std::min(cfg.kingSplitUntil, std::max(rnd, (int)P_DYN_KING_FROM));
    int feedMinUnits = cfg.splitMin >= 999 ? 2 : P_FEED_MINUNITS;   // no-split maps: merge into one king
    unitPeak = std::max(unitPeak, unitCount);
    if (P_FEED_KEEP_FRAC > 0 && cfg.splitMin < 999) {
        // keep enough units alive that the enemy swarm cannot simply wipe us out after the feeding
        int recentEnemies = 0;
        for (auto& g : sightings) if (g.round >= rnd - 40) recentEnemies++;
        feedMinUnits = std::max(feedMinUnits, (int)std::ceil(P_FEED_KEEP_FRAC * std::max(recentEnemies, unitPeak)));
    }
    if (P_E2_DUMP_ROUND > 0 && rnd >= P_E2_DUMP_ROUND) feedMinUnits = 2;   // v5: last rounds: everything goes into the king
    if (dumpRound() > 0 && rnd >= dumpRound()) feedMinUnits = 2;   // v7: same, from the ladder evidence (5 dragons of 14-31 left unmerged)
    bool endFeed = (rnd >= cfg.feedStart || (isolated() && rnd >= P_ISO_FEED_START)) && unitCount >= feedMinUnits;
    bool capFeed = P_CAP_FEED && rnd >= cfg.kingSplitUntil && unitCount >= UNIT_LIMIT - P_CAP_FEED_MARGIN && myLen >= P_CAP_FEED_MINLEN;
    // extra non-farmers merge into the king, unless it is already so long that crowding it is the danger
    int kingLenSeen = (kingK.id >= 0 && kingK.round >= rnd - 60) ? kingK.len : -1;
    for (auto& T : allies) if (T.len > kingLenSeen && T.len > 2 * myLen) kingLenSeen = T.len;
    bool kingRoomy = kingLenSeen >= 0 ? kingLenSeen < P_COURIER_KINGMAX : rnd < P_COURIER_BLIND_UNTIL;
    bool roleFeed = roleMaps() && farmerRole == 0 && rnd >= P_ROLE_FEED_START && kingRoomy;
    // v15 (option): gradual merge on the swarm maps: from P_GRAD_FEED, dragons of P_GRAD_FEED_MINLEN+ feed the king
    // while the short ones go on farming (the ladder's top teams had kings of 56-74 at round 400 there, ours 20-30)
    bool gradFeed = P_GRAD_FEED > 0 && rnd >= P_GRAD_FEED && myLen >= P_GRAD_FEED_MINLEN && localMinMap();
    bool feeding = !iAmKing && (endFeed || capFeed || roleFeed || gradFeed) && huntTile < 0;
    // v16 Trauma heir: a share of the couriers, once P_HEIR_MIN long, keeps its length instead of carrying it
    // to the king (on the ladder both lost Trauma games ended with our only dragon, the king, dead or split up;
    // locally 6 of 8 games against the invading sparring bot were lost by elimination)
    heirNow = P_HEIR && roleMaps() && !isKingFlag && !iAmKing && farmerRole == 0 && myLen >= P_HEIR_MIN && rnd >= P_HEIR_START &&
              (mix64((uint64_t)myId * 2654435761u + 7) % 1000) < (uint64_t)(P_HEIR_FRAC * 1000);
    if (heirNow) feeding = false;
    feedingNow = feeding;
#ifdef DEBUGLOG
    if (getenv("FEED_DEBUG") && myLen >= 12 && rnd >= 420 && rnd % 10 == 0)
        fprintf(stderr, "FEED r%d id%d len%d king%d feeding%d end%d kingK(id%d len%d r%d at %d,%d) units%d huntTile%d\n", rnd, myId, myLen, (int)iAmKing, (int)feeding, (int)endFeed,
                kingK.id, kingK.len, kingK.round, kingK.tile >= 0 ? TX(kingK.tile) : -1, kingK.tile >= 0 ? TY(kingK.tile) : -1, unitCount, huntTile);
#endif
    if (feeding && kingK.id >= 0 && kingK.round >= rnd - 4 && kingK.len >= myLen + feedMargin()) {
        int bt = kingK.tile;
        if (P_E2_PATHDIST) {
            static std::vector<int> ring;
            pathRing(bt, 1, 2, ring);
            for (int w : ring) { feedVal[w] += P_FEED_ATTRACT * 0.3; feedTiles.push_back(w); }
        } else
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++) {
                if (std::abs(dx) + std::abs(dy) > 2 || (dx == 0 && dy == 0)) continue;
                int w = ID(TX(bt) + dx, TY(bt) + dy);
                feedVal[w] += P_FEED_ATTRACT * 0.3; feedTiles.push_back(w);
            }
    }
    if (feeding) {
        for (auto& D : others) {
            if (D.enemy || D.head < 0) continue;
            // feed the king; if it is unknown or far away, feed the biggest dragon at hand instead
            bool kingFar = kingK.id < 0 || kingK.round < rnd - 8 || manhattan(kingK.tile, headT) > P_LOCAL_FEED_DIST;
            bool target = D.id == kingK.id || (kingFar && !feedKingOnly() && !(P_KING_FENCE && roleMaps()));   // v7 plan 3: couriers feed the king only
            if (rvActive()) target = manhattan(D.head, rvTile) <= P_RV_ZONE;   // v5: feed only at the meeting tile
            int blen = std::max(D.vis, D.id == kingK.id ? kingK.len : 0);
            if (!target || blen < myLen + feedMargin()) continue;
            if (D.id != kingK.id && blen < (rvActive() ? P_RV_MINTARGET : localMinTarget())) continue;   // v5: no feeding of small local 'kings
            bigAllies.push_back({D.head, blen});
            // approach to a ring around its head (not adjacent: crowding the head can trap it)
            if (P_E2_PATHDIST) {
                static std::vector<int> ring;
                pathRing(D.head, 2, P_DONATE_DIST, ring);
                for (int w : ring) { feedVal[w] += P_FEED_ATTRACT * 0.5; feedTiles.push_back(w); }
            } else
            for (int dy = -P_DONATE_DIST; dy <= P_DONATE_DIST; dy++)
                for (int dx = -P_DONATE_DIST; dx <= P_DONATE_DIST; dx++) {
                    int md = std::abs(dx) + std::abs(dy);
                    if (md < 2 || md > P_DONATE_DIST) continue;
                    int w = ID(TX(D.head) + dx, TY(D.head) + dy);
                    feedVal[w] += P_FEED_ATTRACT * 0.5; feedTiles.push_back(w);
                }
        }
    }

    // ---- kill mode: the enemy seems almost gone and we clearly outnumber it -> hunt everything
    {
        int recent = 0;
        for (auto& g : sightings) if (g.round >= rnd - P_KILL_WINDOW) recent++;
        killMode = P_KILL && rnd >= P_KILL_MIN_ROUND && recent > 0 && recent <= P_KILL_MAX_ENEMY &&
                   unitCount >= P_KILL_RATIO * recent + 2 && !iAmKing;
    }
    // ---- hunting: converge on the biggest enemy seen recently (a head-on kills it regardless of size)
    if ((cfg.hunt > 0 || killMode) && !iAmKing && myLen >= 2 && (unitCount >= P_HUNT_MINUNITS || killMode)) {
        const Sighting* tg = nullptr;
        for (auto& g : sightings) {
            if (killMode) { if (g.round >= rnd - P_KILL_WINDOW && (!tg || g.round > tg->round || (g.round == tg->round && g.len > tg->len))) tg = &g; }
            else if (g.round >= rnd - P_HUNT_AGE && g.len >= P_HUNT_MIN && g.len >= myLen + P_HUNT_MARGIN && (!tg || g.len > tg->len)) tg = &g;
        }
        if (tg) {
            double v = killMode ? P_HUNT_KILL : cfg.hunt * std::min(1.0, tg->len / 20.0);
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    if (std::abs(dx) + std::abs(dy) > 2) continue;
                    int w = ID(TX(tg->tile) + dx, TY(tg->tile) + dy);
                    feedVal[w] += v * 0.3; feedTiles.push_back(w);
                }
        }
    }

    // v12 skirmish: short dragons go for visible enemy heads that are longer (a head-on gains length),
    // and for equal ones where our heads outnumber theirs (the survivors eat the dropped pearls)
    if (P_SKIRMISH > 0 && (P_SKIRMISH_WHERE == 2 || dbMap >= 0) && (P_SKIRMISH_WHERE != 3 || localRulesOn()) && !iAmKing && myLen >= 2 && myLen <= P_SKIRMISH_MAXLEN &&
        unitCount >= 2 && rnd < cfg.feedPullStart) {
        for (auto& e : enemies) {
            if (!e.lenSure) continue;
            bool fav = e.len > myLen;
            if (!fav && !(P_SKIRMISH_EQ && e.len == myLen)) continue;
            if (!fav) {
                int na = 1, ne = 0;
                for (auto& a : allies) if (manhattan(a.head, e.head) <= 4) na++;
                for (auto& o : enemies) if (o.id != e.id && manhattan(o.head, e.head) <= 4) ne++;
                if (na < ne + 1 + P_SKIRMISH_EQ_MARGIN) continue;
            }
            for (int d = 0; d < 4; d++) {
                int w = nb[e.head][d];
                if (w < 0 || w == e.neck) continue;
                feedVal[w] += P_SKIRMISH; feedTiles.push_back(w);
            }
        }
    }

    // v12 reinforce: go where an ally and an enemy are about to trade, to eat the dropped pearls
    if (P_REINFORCE > 0 && localRulesOn() && !iAmKing && myLen >= 2 && myLen <= 8 && rnd < cfg.feedPullStart) {
        for (auto& al : allies) {
            if (manhattan(al.head, headT) <= 1) continue;
            bool contact = false;
            for (auto& e : enemies) if (manhattan(e.head, al.head) <= 2) { contact = true; break; }
            if (!contact) continue;
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    int md = std::abs(dx) + std::abs(dy);
                    if (md < 2 || md > 2) continue;
                    int w = ID(TX(al.head) + dx, TY(al.head) + dy);
                    feedVal[w] += P_REINFORCE * 0.3; feedTiles.push_back(w);
                }
        }
    }

    // v14 king guard: short dragons go for enemy heads close to our king
    if (P_KING_GUARD > 0 && P_KING_GUARD_PULL > 0 && !isKingFlag)
        for (auto& e : enemies) {
            if (!kingGuardTarget(e)) continue;
            for (int d = 0; d < 4; d++) {
                int w = nb[e.head][d];
                if (w < 0 || w == e.neck) continue;
                feedVal[w] += P_KING_GUARD_PULL; feedTiles.push_back(w);
            }
        }

    if (P_HUNT_BODY > 0 && myLen >= 2 && !(isKing() && rnd >= cfg.kingSplitUntil)) {
        for (auto& D : others) {
            if (!D.enemy || D.head >= 0) continue;
            if (D.vis < std::max(P_HUNT_BODY_MIN, myLen + 3)) continue;
            for (auto& tp : D.tops) {
                int cur = tp.first, f = tp.second;
                for (int k = 0; k < 4; k++) {
                    int nx = nb[cur][f];
                    if (nx < 0) break;
                    cur = nx;
                    feedVal[cur] += P_HUNT_BODY; feedTiles.push_back(cur);
                }
            }
        }
    }

    pmCounter++;
    isKingFlag = isKing();
    queenNow = P_QUEEN_SAFE && myId <= 1;
    buildPockets();
    growMul = 1.0;
    if (cfg.splitMin >= 999 && isKingFlag && myLen >= P_KING_CAP && rnd < P_KING_CAP_UNTIL) growMul = -0.3;   // avoid pearls
    if (myLen >= P_ROOM_MINLEN && (cfg.splitMin >= 999 || P_LONG_ALLMAPS || (P_E2_LONGSAFE && isKingFlag))) {
        // size of the connected area around me (my own body counts as room, other bodies do not)
        static TileArr<int> q; static TileArr<bool> seenR;
        int n = 0, cap = (int)(P_ROOM_FACTOR * myLen) + 2;
        q[n++] = headT; seenR[headT] = true;
        for (int qh = 0; qh < n && n < cap; qh++)
            for (int d = 0; d < 4 && n < cap; d++) {
                int v = nb[q[qh]][d];
                if (v < 0 || seenR[v] || (occId[v] >= 0 && occId[v] != myId)) continue;
                seenR[v] = true; q[n++] = v;
            }
        for (int i = 0; i < n; i++) seenR[q[i]] = false;
        if (n < cap) growMul = std::max(0.0, (double)(n - myLen) / (cap - myLen));
    }
    dangerSrc.clear();
    if (isKingFlag && P_KING_DANGER > 0) {
        buildHome();
        for (auto& g : sightings)
            if (g.round >= rnd - P_DANGER_AGE) dangerSrc.push_back({g.tile, std::max(3, std::min(12, g.len + 1)) + (rnd - g.round) / 2});
    }
    PROF("plans");
    buildHeadField(P_BFS_RADIUS);
    PROF("hf");
    buildGoal();
    PROF("goal");
    if (P_W_PATH > 0) {
        planPaths();
        for (int d = 0; d < 4; d++) G[d] = P_W_DIFF * G[d] + (pathG[d] >= 0 ? P_W_PATH * pathG[d] : 0.0);
    }
    PROF("headfield");
    // long-range pull toward the advertised biggest ally (endgame convergence)
    if (P_FEED_PULL > 0 && !iAmKing && huntTile < 0 && !rvActive() && (rnd >= cfg.feedPullStart || (roleMaps() && farmerRole == 0 && rnd >= P_ROLE_FEED_START - 10 && kingRoomy)) && unitCount >= ((cfg.splitMin >= 999 || (dumpRound() > 0 && rnd >= dumpRound())) ? 2 : P_FEED_MINUNITS) && kingK.id >= 0 &&
        kingK.round >= rnd - 8 && kingK.len >= myLen + feedMargin()) {
        int bt = kingK.tile, best = -1, bd = 1 << 30;
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++) {
                int w = ID(TX(bt) + dx, TY(bt) + dy);
                if (hf.d[w] != UNR && hf.d[w] + std::abs(dx) + std::abs(dy) < bd) { bd = hf.d[w] + std::abs(dx) + std::abs(dy); best = w; }
            }
        if (best >= 0 && bd > 1)
            for (int d = 0; d < 4; d++) if (hf.bits[best] >> d & 1) G[d] += P_FEED_PULL;
    }

    // v5: everybody (king included) walks to the meeting tile
    if (rvActive() && huntTile < 0 && manhattan(rvTile, headT) > P_RV_ZONE / 2) {
        static std::vector<int16_t> rd;
        staticDist(rvTile, rd);
        if (rd[headT] != UNR && rd[headT] > P_RV_ZONE / 2) pullAlong(rd, P_RV_PULL);
    }
    // v12: kings and long dragons meet at our richest start tile (known maps that merge at the end)
    if (P_KMEET > 0 && rnd >= P_KMEET && dbMap >= 0 && cfg.feedStart >= 300 && cfg.splitMin < 999 && huntTile < 0 && !rvActive() &&
        (iAmKing || myLen >= P_KMEET_MINLEN)) {
        int mt = kingHomeTile();
#ifdef DEBUGLOG
        if (getenv("FEED_DEBUG") && rnd % 10 == 0) fprintf(stderr, "KMEET r%d id%d len%d at %d,%d -> %d,%d\n", rnd, myId, myLen, TX(headT), TY(headT), mt >= 0 ? TX(mt) : -1, mt >= 0 ? TY(mt) : -1);
#endif
        if (mt >= 0 && manhattan(mt, headT) > P_KMEET_ZONE) {
            static std::vector<int16_t> kd;
            staticDist(mt, kd);
            if (kd[headT] != UNR && kd[headT] > P_KMEET_ZONE) pullAlong(kd, P_KMEET_PULL);
        }
    }
    // v5 hunters: long-range pull toward the target's last known head
    if (huntTile >= 0 && (P_E2_HUNT_PULL > 0 || loneHunt) && manhattan(huntTile, headT) > 2) {
        static std::vector<int16_t> hd;
        staticDist(huntTile, hd);
        if (hd[headT] != UNR) pullAlong(hd, loneHunt ? P_LONE_PULL : P_E2_HUNT_PULL);
    }
    // couriers (Stronghold/Trauma: neither king nor farmer) walk to the king from anywhere
    if (P_COURIER_PULL > 0 && roleMaps() && !iAmKing &&
        ((farmerRole == 0 && rnd >= P_ROLE_FEED_START - 10 && kingRoomy) || rnd >= cfg.feedPullStart)) {
        int tgt = (kingK.id >= 0 && kingK.round >= rnd - 60) ? kingK.tile : kingHomeTile();
        if (P_KING_FENCE && !(kingK.id >= 0 && kingK.round >= rnd - 8)) { buildKingField(); if (kingAnchor >= 0) tgt = kingAnchor; }   // v7: the king waits there
        if (tgt >= 0 && manhattan(tgt, headT) > P_DONATE_DIST) {
            static std::vector<int16_t> kd;
            staticDist(tgt, kd);
            if (kd[headT] != UNR) pullAlong(kd, P_COURIER_PULL);
        }
    }
    // designated farmers walk to the nearest free pocket
    if (P_POCKET_PULL > 0 && roleMaps() && farmerRole == 1 && farmerOK() && !pocketOf[headT]) {
        const Pocket* bp = nullptr;
        for (auto& P : pockets) {
            if (!P.ours) continue;
            if (P.dist[headT] == UNR || (bp && P.dist[headT] >= bp->dist[headT])) continue;
            bool busy = false;
            for (int u : P.path) if (occId[u] >= 0 && occId[u] != myId) busy = true;
            if (busy) continue;
            bp = &P;
        }
        if (bp && bp->dist[headT] > 0) pullAlong(bp->dist, P_POCKET_PULL);
    }

    // v15 king gate: walk to the loop
    if (kgActive() && !kgIdx[headT]) {
        static std::vector<int16_t> ld; static int ldFor = -1, ldLen = -1;
        if (ldFor != kgLoopFor || ldLen != kgLoopLen) {
            ldFor = kgLoopFor; ldLen = kgLoopLen;
            ld.assign(N, (int16_t)UNR);
            std::vector<int> q;
            for (int t : kgLoop) { ld[t] = 0; q.push_back(t); }
            for (size_t qh = 0; qh < q.size(); qh++) {
                int u = q[qh];
                for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && ld[v] == UNR) { ld[v] = (int16_t)(ld[u] + 1); q.push_back(v); } }
            }
        }
        if (ld[headT] != UNR && ld[headT] > 0) pullAlong(ld, P_KG_PULL);
    }
    // v7 Trauma plan 3: the king waits near the tile where the couriers come home
    if (kingFenceOn() && roleMaps() && isKingFlag && rnd < P_FENCE_UNTIL) {
        buildKingField();
        if (kingAnchor >= 0 && manhattan(kingAnchor, headT) > P_KING_ANCHOR_R) {
            static std::vector<int16_t> ad;
            staticDist(kingAnchor, ad);
            if (ad[headT] != UNR && ad[headT] > P_KING_ANCHOR_R) pullAlong(ad, P_KING_ANCHOR_PULL);
        }
    }
    // early rush toward the best production zone within reach (known maps)
    if (dbMap >= 0) buildZones(); else buildZonesOnline();
    const bool mapZone = dbMap >= 0 && cfg.zonePull > 0;   // v6: per-map zone rush (table columns)
    const double zPull = mapZone ? cfg.zonePull : P_ZONE_PULL, zFrac = mapZone ? cfg.zoneFrac : P_ZONE_FRAC;
    const int zUntil = mapZone ? cfg.zoneUntil : (dbMap >= 0 ? P_ZONE_UNTIL : P_ZONE_ONLINE_UNTIL);
    if (zPull > 0 && !zones.empty() && rnd < zUntil && !isKingFlag && !roleMaps() && !feedingNow &&
        (mix64((uint64_t)myId * 7919u + 17) % 1000) < (uint64_t)(zFrac * 1000)) {
        const Zone* bz = nullptr; double bv = 0;
        const bool centre = zoneCentreMap();
        int bc = 1 << 30;
        for (auto& z : zones) {
            int d = z.dist[headT];
            if (d == UNR) continue;
            if (centre) {   // v13: the zone nearest the map centre
                int c = std::abs(TX(z.center) - W / 2) + std::abs(TY(z.center) - H / 2);
                if (c < bc) { bc = c; bz = &z; bv = 1e9; }
                continue;
            }
            double v = z.rate / (P_ZONE_D0 + d);
            if (v > bv) { bv = v; bz = &z; }
        }
        double here = centre ? 0.0 : zoneRateAt[headT] / P_ZONE_D0;
#ifdef DEBUGLOG
        if (getenv("ZONE_DEBUG") && rnd % 5 == 0) fprintf(stderr, "ZPULL r%d id%d at %d,%d best %d,%d d%d bv %.4f here %.4f\n", rnd, myId, TX(headT), TY(headT), bz ? TX(bz->center) : -1, bz ? TY(bz->center) : -1, bz ? (int)bz->dist[headT] : -1, bv, here);
#endif
        if (bz && bz->dist[headT] > P_ZONE_MINDIST && bv > P_ZONE_MARGIN * here) pullAlong(bz->dist, zPull);
    }
    // v17 centre rush (unknown maps): a share of the dragons (by id) walks toward the middle of the map in the
    // opening; symmetric maps put their contested riches there (Devil's column, Trophy's cup, Default's rooms),
    // and a map we have not seen gives no other hint where to look
    if (P_CENTER_RUSH > 0 && dbMap < 0 && rnd < P_CENTER_UNTIL && !isKingFlag && !iAmKing && !feedingNow && myLen <= P_CENTER_MAXLEN &&
        (mix64((uint64_t)myId * 2654435761u + 7) % 1000) < (uint64_t)(P_CENTER_FRAC * 1000)) {
        static std::vector<int16_t> cd;
        cd.assign(N, (int16_t)UNR);
        std::vector<int> q;
        for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++) { int t = ID(W / 2 + dx, H / 2 + dy); if (cd[t] == UNR) { cd[t] = 0; q.push_back(t); } }
        for (size_t qh = 0; qh < q.size(); qh++) {
            int u = q[qh];
            for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && cd[v] == UNR) { cd[v] = (int16_t)(cd[u] + 1); q.push_back(v); } }
        }
        if (cd[headT] != UNR && cd[headT] > P_CENTER_ARRIVE) pullAlong(cd, P_CENTER_RUSH);
    }
    // v16 zone hold: walk to the zone
    bool holding = holderRole();
    if (holding && !holdIn[headT] && holdDist[headT] != UNR) pullAlong(holdDist, P_HOLD_PULL);
    // v16 sector spread: walk to my home sector
    if (P_SECTOR && dbMap >= 0 && !holding && !isKingFlag && !iAmKing && !feedingNow && rnd >= P_SECTOR_START && rnd < std::min((int)P_SECTOR_UNTIL, cfg.feedPullStart) &&
        myLen <= P_SECTOR_MAXLEN && !roleMaps() && sectorMapOK()) {
        int hs = homeSector();
        if (hs >= 0 && secOf[headT] != hs && secRep[hs] >= 0) {
            static std::vector<int16_t> sd; static int sdFor = -1, sdMap = -3;
            if (sdFor != secRep[hs] || sdMap != dbMap) { sdFor = secRep[hs]; sdMap = dbMap; staticDist(secRep[hs], sd); }
            if (sd[headT] != UNR && sd[headT] > 0) pullAlong(sd, P_SECTOR_PULL);
        }
    }
    // v15 (sparring opponent): a share of the dragons walks into the other team's home (its starting tiles)
    // from round P_INVADE_START, as the ladder's second team did on Trauma (18 portal entries, 13 heads inside)
    if (P_INVADE_FRAC > 0 && dbMap >= 0 && rnd >= P_INVADE_START && !isKingFlag && !iAmKing && myLen >= 2 &&
        (mix64((uint64_t)myId * 3571u + 11) % 1000) < (uint64_t)(P_INVADE_FRAC * 1000)) {
        static std::vector<int16_t> invD; static int invFor = -2;
        int key = dbMap * 2 + mySide();
        if (invFor != key) {
            invFor = key; invD.assign(N, UNR);
            const MapDef& M = MAPDB[dbMap];
            int ti = mySide(), p = 0;
            std::vector<int> q;
            for (int i = 0; i < M.ndr; i++) { if (M.drg[p] != ti) { invD[M.drg[p + 2]] = 0; q.push_back(M.drg[p + 2]); } p += 2 + M.drg[p + 1]; }
            std::vector<char> efield;
            if (P_INVADE_DOOR && !q.empty()) {
                // through the door the other team's couriers use: the landing of its home field whose portal
                // comes out nearest to its farm pocket (on the ladder 17 of the 18 entries came that way)
                efield.assign(N, 0);
                std::vector<int> f = {q[0]}; efield[q[0]] = 1;
                for (size_t i = 0; i < f.size(); i++)
                    for (int d = 0; d < 4; d++) { int v = nb[f[i]][d]; if (v >= 0 && !viaPortal[f[i]][d] && !efield[v]) { efield[v] = 1; f.push_back(v); } }
                int bestL = -1, bs = 1 << 30;
                for (int t : f)
                    for (int d = 0; d < 4; d++) {
                        int o = nb[t][d];
                        if (o < 0 || !viaPortal[t][d] || efield[o]) continue;
                        for (auto& P : pockets) if (!P.ours && P.dist[o] != UNR && P.dist[o] < bs) { bs = P.dist[o]; bestL = t; }
                    }
                if (bestL >= 0) {
                    for (int t : q) invD[t] = UNR;
                    q.assign(1, bestL); invD[bestL] = 0;
                }
            }
            for (size_t qh = 0; qh < q.size(); qh++) {
                int u = q[qh];
                for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && invD[v] == UNR && (efield.empty() || !efield[v])) { invD[v] = (int16_t)(invD[u] + 1); q.push_back(v); } }
            }
        }
        if (invD[headT] != UNR && invD[headT] > P_INVADE_ARRIVE) pullAlong(invD, P_INVADE_PULL);
    }
    // v15 Trauma gate: an eligible courier near the block walks to it, unless an ally already holds it
#ifdef DEBUGLOG
    if (getenv("GATE_DEBUG") && roleMaps() && rnd % 10 == 0) { buildGate(); fprintf(stderr, "GATE r%d id%d role%d len%d body%zu king%d head(%d,%d) field%d elig%d in%d land(%d,%d) anchor(%d,%d)\n", rnd, myId, farmerRole, myLen, myBody.size(), (int)isKingFlag, TX(headT), TY(headT), (int)inKingField(headT), (int)gateEligible(), (int)inGate(), gateLand >= 0 ? TX(gateLand) : -1, gateLand >= 0 ? TY(gateLand) : -1, kingAnchor >= 0 ? TX(kingAnchor) : -1, kingAnchor >= 0 ? TY(kingAnchor) : -1); }
#endif
    if (gateEligible() && !inGate()) {
        static std::vector<int16_t> gD; static int gDFor = -3;
        if (gDFor != gateFor) {
            gDFor = gateFor; gD.assign(N, UNR);
            std::vector<int> q;
            for (int b : gateBlk) { gD[b] = 0; q.push_back(b); }
            for (size_t qh = 0; qh < q.size(); qh++) {
                int u = q[qh];
                for (int d = 0; d < 4; d++) { int v = nb[u][d]; if (v >= 0 && gD[v] == UNR) { gD[v] = (int16_t)(gD[u] + 1); q.push_back(v); } }
            }
        }
        bool held = false;
        for (int b : gateBlk) if (visible[b] && occId[b] >= 0 && occId[b] != myId) {
            for (auto& a : allies) if (a.id == occId[b]) held = true;
        }
#ifdef DEBUGLOG
        if (getenv("GATE_DEBUG") && rnd % 10 == 0) fprintf(stderr, "GATEPULL r%d id%d head(%d,%d) gD%d held%d blk(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n", rnd, myId, TX(headT), TY(headT), (int)gD[headT], (int)held, TX(gateBlk[0]), TY(gateBlk[0]), TX(gateBlk[1]), TY(gateBlk[1]), TX(gateBlk[2]), TY(gateBlk[2]), TX(gateBlk[3]), TY(gateBlk[3]));
#endif
        if (!held && gD[headT] != UNR && gD[headT] <= P_GATE_R) pullAlong(gD, P_GATE_PULL);
    }
    rushingNow = false;
    // v11: opening rush (known maps). On the ladder the top teams send a dragon straight to the map's
    // richest zone in the first rounds (Trophy: through the portal into the cup by round 9) and keep
    // it; whoever gets there first holds it. The original dragons of our side nearest to the best
    // zone (by path, portals included) head there from round 0; the others play as usual.
    if (P_OPEN_RUSH > 0 && dbMap >= 0 && cfg.zonePull <= 0 && openRushMap() && firstRound == 0 && rnd < P_OPEN_RUSH_UNTIL && !isKingFlag && !(queenNow && N >= P_QUEEN_HOME_MINN) && !roleMaps() && spawnHead >= 0) {   // (maps with their own zone rush keep it: Schooltime)
        if (zones.empty()) buildZones();
        const MapDef& M = MAPDB[dbMap];
        int ti = mySide();
        const Zone* bz = nullptr; double bv = 0;
        std::vector<std::pair<int, int>> starts;   // (path distance to the zone, start head tile) of our originals
        for (auto& z : zones) {
            int p = 0, dmin = UNR;
            for (int i = 0; i < M.ndr; i++) { if (M.drg[p] == ti) dmin = std::min<int>(dmin, z.dist[M.drg[p + 2]]); p += 2 + M.drg[p + 1]; }
            if (dmin == UNR) continue;
            double v = z.rate / (P_ZONE_D0 + dmin);
            if (v > bv) { bv = v; bz = &z; }
        }
        if (bz) {
            int p = 0;
            for (int i = 0; i < M.ndr; i++) { if (M.drg[p] == ti) starts.push_back({bz->dist[M.drg[p + 2]], M.drg[p + 2]}); p += 2 + M.drg[p + 1]; }
            std::sort(starts.begin(), starts.end());
            bool mine = false;
            for (int i = 0; i < (int)starts.size() && i < P_OPEN_RUSH_N; i++) if (starts[i].second == spawnHead) mine = true;
            if (mine && bz->dist[headT] != UNR && bz->dist[headT] > P_OPEN_RUSH_ARRIVE) { pullAlong(bz->dist, P_OPEN_RUSH); rushingNow = true; }
            // v13: the next original (the nearest of the others to it) heads for the best other zone, from its own
            // start (Trophy: our crescent; on the ladder the other team took both crescents while all our dragons sat in the cup)
            if (!mine && P_OPEN_RUSH_SPREAD && (int)starts.size() > P_OPEN_RUSH_N) {
                const Zone* z2 = nullptr; int who = -1; double v2 = 0;
                for (auto& z : zones) {
                    if (&z == bz) continue;
                    for (int i = P_OPEN_RUSH_N; i < (int)starts.size(); i++) {
                        int d = z.dist[starts[i].second];
                        if (d == UNR) continue;
                        double v = z.rate / (P_ZONE_D0 + d);
                        if (v > v2) { v2 = v; z2 = &z; who = starts[i].second; }
                    }
                }
                if (z2 && who == spawnHead && z2->dist[headT] != UNR && z2->dist[headT] > P_OPEN_RUSH_ARRIVE) pullAlong(z2->dist, P_OPEN_RUSH);
            }
        }
    }

    kingDropOn = P_KING_DROP_PEN > 0 && !isKingFlag && !iAmKing && rnd >= cfg.feedPullStart && kingK.id >= 0 && kingK.id != myId && kingK.round >= rnd - 3;
#ifdef DEBUGLOG
    if (getenv("KDROP_DEBUG") && rnd >= cfg.feedPullStart && rnd % 20 == 0) fprintf(stderr, "KDROPST r%d id%d king%d iam%d kd%d kingK%d\n", rnd, myId, (int)isKingFlag, (int)iAmKing, kingDist != nullptr, kingK.id);
#endif
    std::vector<Cand> cands;
    for (int d = 0; d < 4; d++) {
        if (nb[headT][d] < 0) continue;
        if (myBody.size() >= 2 && nb[headT][d] == myBody[1]) continue;
        Cand c; c.dirs = {d};
        evaluateMove(c);
        c.why = "move";
        cands.push_back(c);
    }
    // v15 Trauma gate: hold the block by circling it
    bool gateHold = gateEligible() && inGate();
    if (gateHold) {
        int k = gateIdx(headT), nxt = -1;
        int n1 = gateBlk[(k + 1) % 4], n2 = gateBlk[(k + 3) % 4];
        int neck = myBody.size() >= 2 ? myBody[1] : -1;
        nxt = (n1 != neck) ? n1 : n2;
        if (nxt >= 0 && occId[nxt] < 0)
            for (int d = 0; d < 4; d++)
                if (nb[headT][d] == nxt) {
                    Cand c; c.dirs = {d};
                    SimState st = simulate(c.dirs);
                    if (st.alive) { c.score = 950; c.why = "gate"; cands.push_back(c); }
                    break;
                }
    }
    double bestSingle = -1e18;
    for (auto& c : cands) bestSingle = std::max(bestSingle, c.score);
    PROF("moves");
    // last resort: reversing into my own neck kills only me (never take an ally with me)
    if (myBody.size() >= 2)
        for (int d = 0; d < 4; d++)
            if (nb[headT][d] == myBody[1]) {
                Cand c; c.dirs = {d}; c.why = "selfend"; c.score = -1.2e15;
                cands.push_back(c);
                break;
            }
    bool kingSafe = P_E2_KING_SAFE && isKingFlag && rnd >= cfg.feedPullStart - P_E2_LEAD;
    for (int d = 0; d < 4; d++) {
        if (nb[headT][d] >= 0 || !viaPortal[headT][d]) continue;
        Cand c; c.dirs = {d}; c.why = "portal";
        c.score = P_PORTAL_EXPLORE - portalBlind() - (kingSafe ? 20000.0 : 0.0);
        cands.push_back(c);
    }

    // escape sprints
    if (bestSingle < -10000 && myLen >= 3) {
        for (int a = 0; a < 4; a++)
            for (int b = 0; b < 4; b++) {
                Cand c; c.dirs = {a, b};
                evaluateMove(c);
                c.why = "escape2";
                cands.push_back(c);
                if (myLen >= 4)
                    for (int e = 0; e < 4; e++) {
                        Cand c3; c3.dirs = {a, b, e};
                        evaluateMove(c3);
                        c3.why = "escape3";
                        cands.push_back(c3);
                    }
            }
    }

    // v17 evasive sprints: a long dragon (the king above all) that a shorter enemy can strike this turn also
    // weighs 2- and 3-step moves (each extra step costs a segment unless it eats a pearl): on the ladder 34 of our
    // 15+ long dragons in 110 games were struck by a short enemy while making an ordinary one-step move
    if (P_EVADE_SPRINT && (dbMap < 0 || P_EVADE_KNOWN) && bestSingle >= -10000 && (myLen >= P_EVADE_MINLEN || (isKingFlag && myLen >= 3))) {
        bool threat = false;
        for (auto& e : enemies) {
            if (e.len >= myLen) continue;
            int d = e.dist->d[headT];
            if (d != UNR && d <= e.reach + 2) threat = true;
            if (e.ps && e.ps->d[headT] != UNR) threat = true;
        }
        if (threat)
            for (int a = 0; a < 4; a++) {
                if (nb[headT][a] < 0 || (myBody.size() >= 2 && nb[headT][a] == myBody[1])) continue;
                for (int b = 0; b < 4; b++) {
                    Cand c; c.dirs = {a, b};
                    evaluateMove(c);
                    c.why = "evade2";
                    cands.push_back(c);
                    if (P_EVADE_SPRINT >= 2 && myLen >= 4)
                        for (int e3 = 0; e3 < 4; e3++) {
                            Cand c3; c3.dirs = {a, b, e3};
                            evaluateMove(c3);
                            c3.why = "evade3";
                            cands.push_back(c3);
                        }
                }
            }
    }
    PROF("escapes");
    // kamikaze strikes
    bool doomed = bestSingle < -10000;
    // v9: "doomed" above also means "no move keeps a long dragon safe for its whole length" (a dead end
    // that still gives it 15+ moves, or a trap penalty). On the ladder v8 dragons of 17-60 threw themselves
    // at 2-10-long enemies in that state, a Trauma lone king among them. A long dragon trades itself for a
    // shorter enemy only if every move leaves it fewer than P_DOOM_SURV moves.
    bool doomedHard = doomed;
    if (P_DOOM_STRICT_LEN > 0 && myLen >= P_DOOM_STRICT_LEN)
        doomedHard = bestSingle <= -(20000.0 - 500.0 * P_DOOM_SURV);
    for (auto& e : enemies) {
        int maxSteps = std::min(myLen - 1, P_ATTACK_MAX_STEPS);
        if (maxSteps < 1) continue;
        std::vector<int> path;
        if (manhattan(e.head, headT) <= maxSteps) path = attackPath(e.head, maxSteps);
        // v15: a sprint through pearls reaches farther than length - 1
        if (path.empty() && psOn(2) && manhattan(e.head, headT) <= P_PSPRINT_MAX)
            path = psAttackPath(e.head, P_PSPRINT_MAX);
        if (path.empty()) continue;
        SimState s = simulate(path);
        if (s.alive || !s.headOn || s.headOnId != e.id) continue;
        double gain = e.len - myLen;
        bool dm = gain < 0 ? doomedHard : doomed;
        bool allowed = (unitCount >= 2 || dm) && (gain >= P_ATTACK_MIN_GAIN || dm || killMode || loneHunt);
        if (isKing() && !dm && gain < 0) allowed = false;
        // v7.1: a long dragon does not throw itself at a shorter enemy (on the ladder kings of 27-53 did so in
        // the endgame, doomed or in kill mode); when doomed a rescue split keeps most of it
        if (P_ATTACK_LONG_MIN > 0 && myLen >= P_ATTACK_LONG_MIN && gain < P_ATTACK_MIN_GAIN) allowed = false;
        // v14 king guard: a short dragon strikes an enemy close to our king even at a loss of length
        if (!allowed && kingGuardTarget(e)) allowed = true;
        // v14: on the elimination maps, a trade that loses a little length where our heads outnumber theirs
        // (a unit for a unit, and we eat the drops; the top team struck our shorter dragons 77 times in 10 games)
        if (!allowed && P_ATTACK_UNFAV > 0 && gain < 0 && -gain <= P_ATTACK_UNFAV && unitCount >= 2 && !isKing() && localRulesOn() && !killMode) {
            int na = 0, ne = 0;
            for (auto& o : allies) if (manhattan(o.head, e.head) <= P_ATTACK_LOCAL_R) na++;
            for (auto& o : enemies) if (o.id != e.id && manhattan(o.head, e.head) <= P_ATTACK_LOCAL_R) ne++;
            if (na >= ne + P_ATTACK_UNFAV_MARGIN) allowed = true;
        }
        // v12: on the ladder the top team ate 208 of the 232 pearls dropped by trades where it had more heads
        // near the spot: an even trade only where our remaining heads at least match theirs
        // (P_ATTACK_LOCAL 2: also a trade that gains length, unless the enemy is 3x my length: when they eat
        // all the drops, our a-long for their b-long costs us a and them only about b/2 - a/2)
        if (P_ATTACK_LOCAL && allowed && !dm && (gain == 0 || (P_ATTACK_LOCAL == 2 && gain > 0 && e.len < 3 * myLen)) && !killMode && !loneHunt && localRulesOn()) {
            int na = 0, ne = 0;
            for (auto& o : allies) if (manhattan(o.head, e.head) <= P_ATTACK_LOCAL_R) na++;
            for (auto& o : enemies) if (o.id != e.id && manhattan(o.head, e.head) <= P_ATTACK_LOCAL_R) ne++;
            if (na < ne + P_ATTACK_LOCAL_MARGIN) allowed = false;
        }
        if (P_E2_KING_NOATTACK && isKingFlag && !doomed && rnd >= cfg.feedPullStart - P_E2_LEAD) allowed = false;   // (off) the king never trades itself in the endgame
        // v16: the opening rusher takes no trade that does not gain length on its way (on the ladder ours traded
        // 3 for 3 with the other team's rusher at round 8 and the other team's next dragon took Trophy's cup)
        if (P_RUSH_NOTRADE && rushingNow && !dm && gain <= 0) allowed = false;
        if (!allowed) continue;
        Cand c; c.dirs = path; c.why = "attack";
        c.score = 1000 + gain - 0.01 * path.size();
        cands.push_back(c);
    }

    PROF("attacks");
    // donation: die next to a bigger ally's head so it can eat our pearls
    bool invaderSeen = false;   // v15: an enemy inside our Trauma home: keep this dragon as a guard, do not donate
    if (P_HOME_GUARD && P_KING_GUARD > 0) for (auto& e : enemies) if (kingGuardTarget(e)) invaderSeen = true;
    if (!bigAllies.empty() && myBody.size() >= 2 && !invaderSeen && !gateHold && !heirNow) {
        // donate once close enough, unless the big dragon already has plenty to eat around it
        bool adj = false;
        for (auto& b : bigAllies) {
            if (manhattan(b.head, headT) > P_DONATE_DIST) continue;
            if (P_E2_PATHDIST && pathDist(headT, b.head, P_DONATE_DIST) > P_DONATE_DIST) continue;   // v6: reachable, not just close
            int near = 0;
            for (int k = 0; k < 49; k++)
                if (vtile[k].pearl && manhattan(ID(vtile[k].x, vtile[k].y), b.head) <= 3) near++;
            if (near < P_DONATE_MAXPEARLS || (roleMaps() && farmerRole == 0)) adj = true;   // couriers: deliver, do not linger
        }
        if (adj) {
            for (int d = 0; d < 4; d++)
                if (nb[headT][d] == myBody[1]) {
                    Cand c; c.dirs = {d}; c.why = "donate";
                    c.score = 900;
                    cands.push_back(c);
                    break;
                }
        }
    }

    // farm pocket: at its end, leave a 2-long stub and walk out with the rest
    if (doomed && P_POCKET && pocketSplitNow()) {
        int k = myLen - 2;
        while (k > 2 && !kParityOK(k)) k--;
        Cand c; c.split = k; c.why = "pocket";
        c.score = -3000;
        cands.push_back(c);
    }
    // v9: a long dragon (a king above all) splits itself up only when its best move leaves it fewer than
    // P_RESCUE_SURV moves of room: "no move gives room for the whole body" alone made 36-42-long kings on
    // Slithery Fight split in two at round 313 and shred the head half into 2-long units
    if (P_RESCUE_STRICT_LEN > 0 && myLen >= P_RESCUE_STRICT_LEN && doomed && bestSingle > -(20000.0 - 500.0 * P_RESCUE_SURV))
        doomed = false;
    // rescue split: when doomed, save as much of the body as possible as a child
    if (doomed && myLen >= 4 && unitCount < UNIT_LIMIT && (int)myBody.size() < myLen && turnCount <= P_RESCUE_BLIND_TURNS && firstRound == 0) {
        // body partly out of view: split off the largest child blind; it can chain-rescue itself
        Cand c; c.split = myLen - 2; c.why = "rescueB";
        while (c.split > 2 && !kParityOK(c.split)) c.split--;
        c.score = -5000 + c.split;
        cands.push_back(c);
    }
    if (doomed && myLen >= 4 && unitCount < UNIT_LIMIT && (int)myBody.size() == myLen) {
        int L = myLen;
        bool found = false;
        // prefer cutting off a small tail so the head part (which knows its whole body) keeps the length
        if (P_RESCUE_PARENT && L >= P_RESCUE_PARENT_MINLEN)
            for (int k = 2; k <= L - 2 && !found; k++) {
                if (!kParityOK(k)) continue;
                std::vector<int> pb(myBody.begin(), myBody.begin() + (L - k));
                for (int i = L - k; i < L; i++) newMine[myBody[i]] = BIG_FREE;
                int dep = std::min(L - k + 2, isKingFlag ? P_KING_SURV_DEPTH : P_SURV_DEPTH);
                int ps = survival(pb, L - k, 0, dep, isKingFlag ? P_KING_DFS_LIMIT / 2 : 6000, newMine.p);
                for (int i = L - k; i < L; i++) newMine[myBody[i]] = 0;
                if (ps >= dep) {
                    Cand c; c.split = k; c.why = "rescueP";
                    c.score = -4900 + (L - k);
                    cands.push_back(c);
                    found = true;
                }
            }
        // how much of a child (tail part) the child itself would see as one chain from its head
        int visChain = 0;
        for (int i = L - 1; i >= 0 && cheb(myBody[i], myBody[L - 1]) <= 3; i--) visChain++;
        for (int k = L - 2; k >= 2 && !found; k--) {
            if (!kParityOK(k)) continue;
            std::vector<int> cb;
            for (int i = L - 1; i >= L - k; i--) cb.push_back(myBody[i]);
            for (int i = 0; i < L - k; i++) newMine[myBody[i]] = BIG_FREE;
            int cdep = std::min(k + 2, P_SURV_DEPTH);
            int cs = survival(cb, k, 0, cdep, 3000, newMine.p);
            for (int i = 0; i < L - k; i++) newMine[myBody[i]] = 0;
            if (cs >= cdep) {
                // a child that cannot see its whole body may run into it: prefer fully visible children
                Cand c; c.split = k; c.why = "rescue";
                c.score = (k <= visChain ? -5000 : -5400) + k;
                cands.push_back(c);
                if (k <= visChain) found = true;
            }
        }
        if (!found && P_RESCUE_X) {
            // no child survives for sure: still save the largest child, it may rescue itself
            Cand c; c.split = L - 2; c.why = "rescueX";
            while (c.split > 2 && !kParityOK(c.split)) c.split--;
            c.score = -6000 + c.split;
            cands.push_back(c);
        }
    }

    PROF("rescue");
    // a farmer that has grown long sends its tail part to the king as a courier and stays short
    if (P_COURIER_LEN > 0 && roleMaps() && farmerRole == 1 && !pocketOf[headT] && myLen >= P_COURIER_LEN && kingRoomy &&
        bestSingle > -1000 && rnd < cfg.feedPullStart) {
        // the tail part becomes another farmer while we have fewer than P_FARMERS, else a courier
        int want = unitCount < 1 + P_FARMERS ? 1 : 0;
        int k = myLen - P_FARMER_KEEP;
        while (k >= 2 && roleOfLen(k) != want) k--;   // farmers odd, couriers 2 mod 4
        if (k >= 2 && myLen - k >= 3 && courierSplitOK(k)) {
            Cand c; c.split = k; c.why = want ? "farmer2" : "courier";
            c.score = 400;
            cands.push_back(c);
        }
    }
    // split
    if (wantSplit() && bestSingle > -1000) {
        int k = splitSize();
        if (splitOK(k)) {
#ifdef DEBUGLOG
            if (getenv("KING_DEBUG") && myLen >= 10) fprintf(stderr, "KSPLIT r%d id%d len%d k%d iAmKing%d kingK(id%d len%d r%d) units%d\n", rnd, myId, myLen, k, (int)iAmKing, kingK.id, kingK.len, kingK.round, unitCount);
#endif
            Cand c; c.split = k; c.why = "split";
            c.score = 500;
            cands.push_back(c);
        }
    }

    // v7 Trauma plan 3: a farmer walking out of its pocket turns its tail into the next farmer as soon as
    // the tail reaches the pocket mouth (the child's head is there, facing into the pocket); the rest walks
    // on to the king as a courier
    if (P_POCKET_REVERSE && roleMaps() && farmerRole == 1 && !pocketOf[headT] && (int)myBody.size() == myLen &&
        myLen >= 5 && unitCount < UNIT_LIMIT && rnd < cfg.feedPullStart) {
        int tail = myBody.back();
        // my tail at the pocket mouth: the first tiles of the pocket or its junction (children need a turn
        // or two to recognise the map, so the very first tile is often missed)
        bool mouth = pocketOf[tail] && pocketPos[tail] <= 2;
        if (!mouth && !pocketOf[myBody[myLen - 2]]) mouth = false;
        else if (!mouth) { int pi = pocketOf[myBody[myLen - 2]]; mouth = pocketPos[myBody[myLen - 2]] == 0 && pockets[pi - 1].junction == tail; }
        if (mouth) {
            int k = myLen - 4;                    // courier of 4 (or 5)
            if (k < 3) k = 3;
            if (!(k & 1)) k--;                    // odd: the child is a farmer
            if (k >= 3 && myLen - k >= 2) {
                Cand c; c.split = k; c.why = "reverse";
                c.score = 650;
                cands.push_back(c);
            }
        }
    }
    if (P_BOX_ONEWAY && P_LANDING_PEN > 0 && oneWayCur) {
        // v8: do not park on the tile where units leaving a box come out
        for (auto& c : cands) {
            if (c.split || c.dirs.empty()) continue;
            int t = headT;
            for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
            if (isLanding(t)) c.score -= P_LANDING_PEN * (myLen + 2);
        }
    }
    if (P_PORTAL_PROBE && probeHit >= 0) {
        for (auto& c : cands) {
            if (c.split || c.dirs.empty() || c.dirs[0] != probeDir || !viaPortal[headT][probeDir]) continue;
            if (probeHit == 1) c.score -= P_PROBE_PEN * (myLen + 2);
        }
    }
    if (P_PORTAL_RISK > 0 || P_PORTAL_RISK_KING > 0) {
        // v7.1: the tile behind a portal is out of sight; on the ladder 10-25% of our crossings met an ally's
        // (or enemy's) head there. Charge a move through a portal with a blind exit for that risk.
        for (auto& c : cands) {
            if (c.split || c.dirs.empty() || strcmp(c.why, "move")) continue;
            int d0 = c.dirs[0], x = nb[headT][d0];
            if (x < 0 || !viaPortal[headT][d0] || visible[x]) continue;
            if (P_PORTAL_PROBE && probeHit == 0 && d0 == probeDir) continue;   // v8: the probe saw it empty
            bool bigOne = isKingFlag || (P_PORTAL_RISK_LEN > 0 && myLen >= P_PORTAL_RISK_LEN);
            // v15: on Portals the long dragons pay P_PORTALS_KRISK (the ladder's 44-long king crossed no portal in
            // its last 150 rounds and ate 2 pearls in the last 75, while the other team's king grew 10)
            double kr = (P_PORTALS_KRISK >= 0 && onPortalsMap() && rnd < P_PORTALS_KRISK_END) ? (double)P_PORTALS_KRISK : (double)P_PORTAL_RISK_KING;
            c.score -= (bigOne ? kr : P_PORTAL_RISK) * (myLen + 2);
        }
    }
    if (P_UNIT_FENCE && P_KING_FENCE && roleMaps() && !isKingFlag && rnd < cfg.feedPullStart) {
        // farmers and couriers stay in the home field and the pocket's region (the other portals lead into
        // the maze: small rooms that trap them, and the enemy)
        buildKingField();
        if (teamField.size() == (size_t)N && teamField[headT])
            for (auto& c : cands) {
                if (c.split || c.dirs.empty()) continue;
                int t = headT;
                for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
                if (!teamField[t]) c.score -= 60;
            }
    }
    if (kingFenceOn() && roleMaps() && isKingFlag && rnd < P_FENCE_UNTIL) {
        // the king stays in its home field (the portals lead to the farmers' pocket and to the enemy)
        buildKingField();
        for (auto& c : cands) {
            if (c.split || c.dirs.empty()) continue;
            int t = headT;
            for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
            if (kingField.size() == (size_t)N && !kingField[t]) c.score -= 60;
        }
    }
    // v16 zone hold: inside the zone, a move that leaves it costs P_HOLD_STAY
    if (holderRole() && holdIn[headT])
        for (auto& c : cands) {
            if (c.split || c.dirs.empty()) continue;
            int t = headT;
            for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
            if (!holdIn[t]) c.score -= P_HOLD_STAY;
        }
    // v17 online hold (unknown maps): a small dragon at a rich spot (fast-respawning tiles seen around it) does
    // not wander off; a move to a spot with less than half the production costs P_OHOLD_STAY (the known-map hold
    // on Devil won 85 of 96 local games against 55)
    if (P_OHOLD_STAY > 0 && P_ZONE_ONLINE > 0 && dbMap < 0 && (P_OHOLD_MAXN <= 0 || N < P_OHOLD_MAXN) && !isKingFlag && !iAmKing && !feedingNow && myLen <= P_OHOLD_MAXLEN &&
        rnd < cfg.feedPullStart && zoneRateAt[headT] >= P_OHOLD_RATE &&
        (mix64((uint64_t)myId * 1299709u + 3) % 1000) < (uint64_t)(P_OHOLD_FRAC * 1000))
        for (auto& c : cands) {
            if (c.split || c.dirs.empty()) continue;
            int t = headT;
            for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
            if (zoneRateAt[t] < 0.5 * zoneRateAt[headT]) c.score -= P_OHOLD_STAY;
        }
    // v15 king gate: on the loop, keep going round it (the way the neck is not)
    if (kgActive() && kgIdx[headT]) {
        int P = (int)kgLoop.size(), i = kgIdx[headT] - 1;
        int nx = kgLoop[(i + 1) % P], pv = kgLoop[(i + P - 1) % P];
        int tgt = (myBody.size() > 1 && myBody[1] == nx) ? pv : nx;
        for (auto& c : cands) {
            if (c.split || c.dirs.size() != 1) continue;
            if (nb[headT][c.dirs[0]] == tgt) c.score += P_KG_BONUS;
        }
#ifdef DEBUGLOG
        if (getenv("KG_DEBUG")) fprintf(stderr, "KG r%d id%d len%d head(%d,%d) idx%d/%d tgt(%d,%d)\n", rnd, myId, myLen, TX(headT), TY(headT), i, P, TX(tgt), TY(tgt));
#endif
    }
    // v18: the queen (dragon 0/1) decides round 500 first, before the longest dragon. On the ladder ours died in
    // 36 of 39 games, mostly by her own choice: left as the stub of a pocket split, donated, or traded head-on.
    if (queenNow)
        for (auto& c : cands)
            if (!strcmp(c.why, "donate") || !strcmp(c.why, "pocket") || !strcmp(c.why, "attack")) c.score -= P_QUEEN_NODIE;
    // v18: give our queen room: in local games she died boxed in by our own dragons (every move fatal) or hit
    // head-on by one of them; non-queen dragons pay for ending a move next to her head
    if (!queenNow && P_QUEEN_ROOM_PEN > 0)
        for (auto& a : allies) {
            if (a.id > 1) continue;
            for (auto& c : cands) {
                if (c.split || c.dirs.empty() || c.score < -1e9) continue;
                int t = headT;
                for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
                int dd = manhattan(t, a.head);
                if (dd <= P_QUEEN_ROOM_R) c.score -= P_QUEEN_ROOM_PEN * (P_QUEEN_ROOM_R + 1 - dd);
            }
        }
    // v18: the rank-1 team's queen stays near her spawn inside her own swarm (alive at the end in 70% of its
    // games against our earlier bots); ours roamed into the fights. Pull her home beyond P_QUEEN_HOME_R.
    if (queenNow && P_QUEEN_HOME_R > 0 && spawnHead >= 0 && N >= P_QUEEN_HOME_MINN) {
        int d0 = manhattan(headT, spawnHead);
        for (auto& c : cands) {
            if (c.split || c.dirs.empty() || c.score < -1e9) continue;
            int t = headT;
            for (int d : c.dirs) { int v = nb[t][d]; if (v < 0) break; t = v; }
            int dd = manhattan(t, spawnHead);
            if (dd > P_QUEEN_HOME_R && dd > d0) c.score -= P_QUEEN_HOME_PULL * (dd - P_QUEEN_HOME_R);
        }
    }
    Cand* best = nullptr;
    for (auto& c : cands) if (!best || c.score > best->score) best = &c;
#ifdef DEBUGLOG
    {
        static int dbgId = getenv("DRAGON_DEBUG") ? atoi(getenv("DRAGON_DEBUG")) : -1;
        if (dbgId == -2 && best) fprintf(stderr, "C r%d id%d len%d %s %.2f%s king=%d@%d\n", rnd, myId, myLen, best->why, best->score, iAmKing ? " KING" : "", kingK.id, kingK.round);
        if (dbgId == myId && getenv("DEBUG_TILE")) {
            int x, y; sscanf(getenv("DEBUG_TILE"), "%d,%d", &x, &y);
            int t = ID(x, y);
            ensureModel(t);
            fprintf(stderr, "   tile(%d,%d) vis=%d pearl=%d cdR=%d cdV=%d pR=%d pV=%d pv=%.2f spawnIn=%d gap=[%d,%d] r1=%.2f r3=%.2f\n", x, y, visible[t], pearlNow[t],
                    tk[t].cdR, tk[t].cdV, tk[t].pR, tk[t].pV, pvNow[t], spawnIn[t], gapA[t], gapB[t], rewardAt(t, 1), rewardAt(t, 3));
        }
        if (dbgId == myId && getenv("DRAGON_DEBUG_MAP")) {
            // value map: pearl probability (0-9) of each tile in the head field, '#' unreachable, '@' head
            for (int y = 0; y < H; y++) {
                std::string row;
                for (int x = 0; x < W; x++) {
                    int t = ID(x, y);
                    if (t == headT) { row += '@'; continue; }
                    if (hf.d[t] == UNR) { row += (occId[t] >= 0 ? 'x' : '#'); continue; }
                    double v = pvNow[t];
                    if (spawnIn[t] >= 0) row += 's';
                    else row += v <= 0 ? '.' : (char)('0' + std::min(9, (int)(v * 10)));
                }
                fprintf(stderr, "   %s\n", row.c_str());
            }
        }
        if (dbgId == myId || dbgId == 9999) {
            fprintf(stderr, "[r%d id%d len%d head(%d,%d) body=%d] G=%.2f %.2f %.2f %.2f\n", rnd, myId, myLen, hx, hy, (int)myBody.size(), G[0], G[1], G[2], G[3]);
            for (auto& c : cands) {
                std::string mv;
                for (int d : c.dirs) mv += DCH[d];
                fprintf(stderr, "   %-8s %-4s split=%d score=%.2f\n", c.why, mv.c_str(), c.split, c.score);
            }
        }
    }
#endif
    char tmp[256];
#ifdef DEBUGLOG
    if (getenv("KDBG") && rnd >= atoi(getenv("KDBG")) && myTeam == 'A')
        fprintf(stderr, "K r%d id%d len%d h%d,%d king=%d kid%d klen%d kage%d kdist%d feed%d big%zu why=%s rv%d,%d@%d hunt%d\n", rnd, myId, myLen, TX(headT), TY(headT),
                (int)iAmKing, kingK.id, kingK.len, kingK.id >= 0 ? rnd - kingK.round : -1, kingK.id >= 0 ? manhattan(kingK.tile, headT) : -1,
                (int)feedingNow, bigAllies.size(), best ? best->why : "-", rvTile >= 0 ? TX(rvTile) : -1, rvTile >= 0 ? TY(rvTile) : -1, rvRound, huntTile >= 0);
#endif
    if (!best) { emit("MOVE N"); return; }
    lastDirs.clear(); lastHead = headT;
    if (best->split) {
        if (roleMaps() && farmerRole == 2 && best->split > myLen - best->split) farmerRole = 0;   // the child is king now
        if (!strcmp(best->why, "reverse")) farmerRole = 0;   // v7: the head part delivers to the king
#ifdef DEBUGLOG
        bool wasKingDbg = iAmKing;
#endif
        if (iAmKing) { iAmKing = false; kingK = {-1, -1, 0, -1}; }   // re-decided next turn from lengths
        if (P_BODY_MSG && (int)myBody.size() == myLen) prepareBodyMsgs(best->split);
        lastSplitRound = rnd;
        snprintf(tmp, sizeof tmp, "SPLIT %d", best->split);
        emit(tmp);
        snprintf(tmp, sizeof tmp, "INDICATOR split %d %s", best->split, best->why);   // v10: which rule split (ladder analysis)
        emit(tmp);
#ifdef DEBUGLOG
        if (getenv("KING_DEBUG") && myLen >= 10) fprintf(stderr, "LSPLIT r%d id%d len%d k%d why=%s bestSingle=%.1f iAmKing(before)=%d kingK(id%d len%d r%d)\n", rnd, myId, myLen, best->split, best->why, bestSingle, (int)wasKingDbg, kingK.id, kingK.len, kingK.round);
#endif
        return;
    }
    std::string mv = "MOVE ";
    for (int d : best->dirs) mv += DCH[d];
#ifdef DEBUGLOG
    if (getenv("PROBE_DBG") && !best->dirs.empty() && viaPortal[headT][best->dirs[0]] && nb[headT][best->dirs[0]] >= 0 && !visible[nb[headT][best->dirs[0]]])
    { int alt = 0; for (auto& c : cands) if (&c != best && c.score > -1e9) alt++;
        fprintf(stderr, "CROSS r%d id%d at(%d,%d) dir%c probed=%d hit=%d why=%s score=%.1f alt=%d\n", rnd, myId, TX(headT), TY(headT), DCH[best->dirs[0]], (int)(probeHit >= 0 && best->dirs[0] == probeDir), probeHit, best->why, best->score, alt); }
#endif
    emit(mv.c_str());
    lastDirs = best->dirs;
    int cur = headT;
    for (int d : best->dirs) {
        int v = nb[cur][d];
        if (v < 0) break;
        lastPath.push_back(v);
        cur = v;
    }
#ifdef CPU_TRACE
    snprintf(tmp, sizeof tmp, "INDICATOR %s %.1f |n%ld b%ld pm%ld s%ld o%ld hf%d c%zu e%zu a%zu sv%ld ct%ld L%d", best->why, best->score, turnNodes, beamWork, cntPM, cntSim, cntBFS, hf.n, cands.size(), enemies.size(), allies.size(), cntSurv, cntCut, myLen);
    cntPM = cntSim = cntBFS = cntSurv = cntCut = 0;
#else
    snprintf(tmp, sizeof tmp, "INDICATOR %s %.1f%s", best->why, best->score, loneHunt ? " hunt" : "");   // v11: "hunt" = hunting a lone enemy king
#endif
    emit(tmp);
}

}  // namespace

// ------------------------------------------------------------------ multi-instance support
// (used only by the local simulator: one process hosts every dragon of a team)
namespace {
struct Inst {
    int myId; char myTeam; int turnCount, firstRound, symKnown; bool symAlive[3];
    std::vector<TileK> tk; std::vector<int16_t> eH, eV; std::vector<char> eHd, eVd;
    std::vector<PortalEnd> portals; std::vector<int> myBody, lastPath;
    std::vector<int32_t> nbs; std::vector<char> vps; bool nbFull;
    std::vector<int> lastDirs; int lastHead;
    std::vector<Sighting> sightings; int gossipCtr; Sighting bestBig; int lastContact; bool kingVeto; int lastSplitRound;
    int dbMap; bool dbTried; uint32_t dbCand, dbCandW; int dbWallsOf;
    Sighting kingK; bool iAmKing, kingInit; int farmerRole; std::vector<int> birthTiles; int birthLen;
    double calA[PC_N], calB[PC_N];
    int unitPeak; int bigEnemySeen;
    int rvTile, rvRound, homeTile, homeId;
    int probeDir, probeFrom, probeRound;
    int sideIdx, spawnHead;
};
void ensureGaps() {
    if (dbMap < 0 || gapFor == dbMap) return;
    const MapDef& M = MAPDB[dbMap];
    for (int t = 0; t < N; t++) {
        int c = dbCls(dbMap, t);
        if (c == 7) { gapA[t] = 0; gapB[t] = 0; } else { gapA[t] = (int16_t)M.clsA[c]; gapB[t] = (int16_t)M.clsB[c]; }
    }
    gapFor = dbMap;
    applyExtraGaps();
}
void resetState() {
    for (int t = 0; t < N; t++) { tk[t] = TileK(); eH[t] = E_UNK; eV[t] = E_UNK; eHd[t] = eVd[t] = false; }
    portals.clear(); myBody.clear(); lastPath.clear(); lastDirs.clear(); lastHead = -1;
    sightings.clear(); gossipCtr = 0; bestBig = {-1, -1, 0, -1}; lastContact = -1; kingVeto = false; lastSplitRound = -9;
    nbFull = true; nbDirty.clear();
    symAlive[0] = symAlive[1] = symAlive[2] = true; symKnown = -1;
    turnCount = 0; firstRound = -1;
    dbMap = -1; dbTried = false; dbCand = 0xFFFFFFFFu; dbCandW = 0xFFFFFFFFu; dbWallsOf = -1; cfg = CFG_DEFAULT;
    kingK = {-1, -1, 0, -1}; iAmKing = false; kingInit = false; farmerRole = -1; birthTiles.clear(); birthLen = 0;
    for (int c = 0; c < PC_N; c++) { calA[c] = calB[c] = 0; }
    updateCalF();
    unitPeak = 0; bigEnemySeen = 0;
    rvTile = -1; rvRound = -1; homeTile = -1; homeId = -1;
    probeDir = -1; probeFrom = -1; probeRound = -9;
    sideIdx = -1; spawnHead = -1;
}
void saveInst(Inst& I) {
    I.myId = myId; I.myTeam = myTeam; I.turnCount = turnCount; I.firstRound = firstRound; I.symKnown = symKnown;
    for (int k = 0; k < 3; k++) I.symAlive[k] = symAlive[k];
    I.tk.assign(tk.p, tk.p + N); I.eH.assign(eH.p, eH.p + N); I.eV.assign(eV.p, eV.p + N);
    I.eHd.assign(eHd.p, eHd.p + N); I.eVd.assign(eVd.p, eVd.p + N);
    I.portals = portals; I.myBody = myBody; I.lastPath = lastPath;
    I.nbs.assign(&nb[0][0], &nb[0][0] + 4 * N); I.vps.assign(&viaPortal[0][0], &viaPortal[0][0] + 4 * N);
    I.nbFull = nbFull; I.lastDirs = lastDirs; I.lastHead = lastHead;
    I.sightings = sightings; I.gossipCtr = gossipCtr; I.bestBig = bestBig; I.lastContact = lastContact; I.kingVeto = kingVeto; I.lastSplitRound = lastSplitRound;
    I.dbMap = dbMap; I.dbTried = dbTried; I.dbCand = dbCand; I.dbCandW = dbCandW; I.dbWallsOf = dbWallsOf;
    I.kingK = kingK; I.iAmKing = iAmKing; I.kingInit = kingInit; I.farmerRole = farmerRole; I.birthTiles = birthTiles; I.birthLen = birthLen;
    for (int c = 0; c < PC_N; c++) { I.calA[c] = calA[c]; I.calB[c] = calB[c]; }
    I.unitPeak = unitPeak; I.bigEnemySeen = bigEnemySeen;
    I.rvTile = rvTile; I.rvRound = rvRound; I.homeTile = homeTile; I.homeId = homeId;
    I.probeDir = probeDir; I.probeFrom = probeFrom; I.probeRound = probeRound;
    I.sideIdx = sideIdx; I.spawnHead = spawnHead;
}
void loadInst(const Inst& I) {
    myId = I.myId; myTeam = I.myTeam; turnCount = I.turnCount; firstRound = I.firstRound; symKnown = I.symKnown;
    for (int k = 0; k < 3; k++) symAlive[k] = I.symAlive[k];
    std::copy(I.tk.begin(), I.tk.end(), tk.p); std::copy(I.eH.begin(), I.eH.end(), eH.p); std::copy(I.eV.begin(), I.eV.end(), eV.p);
    for (int t = 0; t < N; t++) { eHd[t] = I.eHd[t]; eVd[t] = I.eVd[t]; }
    portals = I.portals; myBody = I.myBody; lastPath = I.lastPath;
    std::copy(I.nbs.begin(), I.nbs.end(), &nb[0][0]);
    for (int i = 0; i < 4 * N; i++) (&viaPortal[0][0])[i] = I.vps[i];
    nbFull = I.nbFull; nbDirty.clear(); lastDirs = I.lastDirs; lastHead = I.lastHead;
    sightings = I.sightings; gossipCtr = I.gossipCtr; bestBig = I.bestBig; lastContact = I.lastContact; kingVeto = I.kingVeto; lastSplitRound = I.lastSplitRound;
    dbMap = I.dbMap; dbTried = I.dbTried; dbCand = I.dbCand; dbCandW = I.dbCandW; dbWallsOf = I.dbWallsOf;
    cfg = cfgFor(dbMap >= 0 ? dbMap : dbWallsOf);
    kingK = I.kingK; iAmKing = I.iAmKing; kingInit = I.kingInit; farmerRole = I.farmerRole; birthTiles = I.birthTiles; birthLen = I.birthLen;
    for (int c = 0; c < PC_N; c++) { calA[c] = I.calA[c]; calB[c] = I.calB[c]; }
    updateCalF();
    unitPeak = I.unitPeak; bigEnemySeen = I.bigEnemySeen;
    rvTile = I.rvTile; rvRound = I.rvRound; homeTile = I.homeTile; homeId = I.homeId;
    probeDir = I.probeDir; probeFrom = I.probeFrom; probeRound = I.probeRound;
    sideIdx = I.sideIdx; spawnHead = I.spawnHead;
    ensureGaps();
}
void globalInit() {
    gpow[0] = 1;
    for (int i = 1; i < 1024; i++) gpow[i] = gpow[i - 1] * P_GAMMA;
    fpow[0] = 1;
    for (int i = 1; i < 1024; i++) fpow[i] = fpow[i - 1] * P_FARM_GAMMA;
    for (int i = 0; i < 2048; i++) { decayTab[i] = std::exp(-i / P_DECAY); decay3Tab[i] = std::exp(-i / (3 * P_DECAY)); }
}
void runTurn() {
    turnSerial++;
    turnStart = Clock::now();
    turnNodes = 0;
    if (firstRound < 0) firstRound = rnd;
    out.clear();
#ifdef DEBUGLOG
    profK = 0;
#endif
    PROF("start");
    updateKnowledge();
    if (P_GOSSIP) receiveGossip();
    PROF("knowledge");
    buildNeighbours();
    buildDragons();
    PROF("dragons");
    decide();
    PROF("decide-end");
    if (P_GOSSIP) sendGossip();
    PROF("gossip");
#ifdef DEBUGLOG
    if (getenv("PROF_DEBUG") && myLen >= atoi(getenv("PROF_DEBUG"))) {
        double tot = std::chrono::duration<double, std::micro>(profT[profK - 1] - profT[0]).count();
        if (tot > atof(getenv("PROF_MIN") ? getenv("PROF_MIN") : "2500")) {
            fprintf(stderr, "PROF r%d id%d len%d total%.0f", rnd, myId, myLen, tot);
            for (int i = 1; i < profK; i++) fprintf(stderr, " %s=%.0f", profN[i], std::chrono::duration<double, std::micro>(profT[i] - profT[i - 1]).count());
            fprintf(stderr, " nodes%ld beam%ld\n", turnNodes, beamWork);
        }
    }
#endif
#ifdef DEBUGLOG
    if (getenv("TIME_DEBUG")) {
        double us = std::chrono::duration<double, std::micro>(Clock::now() - turnStart).count();
        if (us > atof(getenv("TIME_DEBUG"))) fprintf(stderr, "TIME r%d id%d len%d %.0fus nodes%ld king%d\n", rnd, myId, myLen, us, turnNodes, (int)isKingFlag);
    }
#endif
    out += "PROTOCOL 3\nENDTURN\n";
    fwrite(out.data(), 1, out.size(), stdout);
    fflush(stdout);
    turnCount++;
}
int multiMain() {
    std::vector<Inst*> insts;
    [[maybe_unused]] int lastTurnId = -1;
    while (readLine()) {
        if (!strcmp(tok[0], "SPAWN")) {
            int id = atoi(tok[1]);
            if (!readInit()) return 0;
            N = W * H;
            allocTiles(N);
            resetState();
            if ((int)insts.size() <= id) insts.resize(id + 1, nullptr);
            insts[id] = new Inst();
            saveInst(*insts[id]);
#if P_SIM_INHERIT
            // simulator-only experiment: a newborn starts with its parent's knowledge of the map
            if (lastTurnId >= 0 && lastTurnId < (int)insts.size() && insts[lastTurnId]) {
                Inst& C = *insts[id]; const Inst& Pa = *insts[lastTurnId];
                C.tk = Pa.tk; C.eH = Pa.eH; C.eV = Pa.eV; C.eHd = Pa.eHd; C.eVd = Pa.eVd; C.portals = Pa.portals;
                C.nbs = Pa.nbs; C.vps = Pa.vps; C.nbFull = true; C.symKnown = Pa.symKnown;
                for (int k = 0; k < 3; k++) C.symAlive[k] = Pa.symAlive[k];
                C.sightings = Pa.sightings; C.bigEnemySeen = Pa.bigEnemySeen;
                for (int c = 0; c < PC_N; c++) { C.calA[c] = Pa.calA[c]; C.calB[c] = Pa.calB[c]; }
            }
#endif
        } else if (!strcmp(tok[0], "TURN")) {
            int id = atoi(tok[1]);
            lastTurnId = id;
            loadInst(*insts[id]);
            if (!readRound()) return 0;
            runTurn();
            saveInst(*insts[id]);
        } else if (!strcmp(tok[0], "KILL")) {
            int id = atoi(tok[1]);
            if (id < (int)insts.size() && insts[id]) { delete insts[id]; insts[id] = nullptr; }
        }
    }
    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
    globalInit();
    if (argc > 1 && !strcmp(argv[1], "--multi")) return multiMain();
    if (!readInit()) return 0;
    N = W * H;
    allocTiles(N);
    cfg = cfgFor(-1);
    while (readRound()) runTurn();
    return 0;
}
