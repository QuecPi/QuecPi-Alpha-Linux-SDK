/*
 * DO NOT EDIT -- file generated from data in db.txt
 */

#include <linux/nl80211.h>
#include <net/cfg80211.h>
#include "regdb.h"

static const struct ieee80211_regdomain regdom_00 = {
	.alpha2 = "00",
	.reg_rules = {
		REG_RULE(755, 928, 2, 0, 20, 0),
		REG_RULE(2402, 2472, 40, 0, 20, 0),
		REG_RULE(2457, 2482, 20, 0, 20, 0),
		REG_RULE(2474, 2494, 20, 0, 20, 
			NL80211_RRF_NO_OFDM | 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 20, 0),
		REG_RULE(57240, 63720, 2160, 0, 0, 0),
	},
	.n_reg_rules = 9
};

static const struct ieee80211_regdomain regdom_AD = {
	.alpha2 = "AD",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_AE = {
	.alpha2 = "AE",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_AF = {
	.alpha2 = "AF",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AI = {
	.alpha2 = "AI",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AL = {
	.alpha2 = "AL",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_AM = {
	.alpha2 = "AM",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 18, 0),
		REG_RULE(5250, 5330, 20, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_AN = {
	.alpha2 = "AN",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AR = {
	.alpha2 = "AR",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_AS = {
	.alpha2 = "AS",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_AT = {
	.alpha2 = "AT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_AU = {
	.alpha2 = "AU",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 36, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5600, 80, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5650, 5730, 80, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5730, 5850, 80, 0, 36, 0),
		REG_RULE(5925, 6425, 160, 0, 24, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 43, 
			NL80211_RRF_NO_OUTDOOR | 0),
	},
	.n_reg_rules = 8
};

static const struct ieee80211_regdomain regdom_AW = {
	.alpha2 = "AW",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_AZ = {
	.alpha2 = "AZ",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 18, 0),
		REG_RULE(5250, 5330, 80, 0, 18, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_BA = {
	.alpha2 = "BA",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_BB = {
	.alpha2 = "BB",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BD = {
	.alpha2 = "BD",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_BE = {
	.alpha2 = "BE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_BF = {
	.alpha2 = "BF",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_BG = {
	.alpha2 = "BG",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_BH = {
	.alpha2 = "BH",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BL = {
	.alpha2 = "BL",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BM = {
	.alpha2 = "BM",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_BN = {
	.alpha2 = "BN",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BO = {
	.alpha2 = "BO",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_BR = {
	.alpha2 = "BR",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 27, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 27, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 30, 0),
		REG_RULE(5925, 7125, 320, 0, 12, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 71000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_BS = {
	.alpha2 = "BS",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_BT = {
	.alpha2 = "BT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BY = {
	.alpha2 = "BY",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_BZ = {
	.alpha2 = "BZ",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_CA = {
	.alpha2 = "CA",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5600, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5650, 5730, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
		REG_RULE(5925, 7125, 320, 0, 12, 
			NL80211_RRF_NO_OUTDOOR | 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_CF = {
	.alpha2 = "CF",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 17, 0),
		REG_RULE(5250, 5330, 40, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 40, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 40, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_CH = {
	.alpha2 = "CH",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 71000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_CI = {
	.alpha2 = "CI",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_CL = {
	.alpha2 = "CL",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CN = {
	.alpha2 = "CN",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5350, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 33, 0),
		REG_RULE(57240, 59400, 2160, 0, 28, 0),
		REG_RULE(59400, 63720, 2160, 0, 44, 0),
		REG_RULE(63720, 65880, 2160, 0, 28, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_CO = {
	.alpha2 = "CO",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_CR = {
	.alpha2 = "CR",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 17, 0),
		REG_RULE(5250, 5330, 20, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 20, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_CU = {
	.alpha2 = "CU",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 23, 0),
		REG_RULE(5150, 5350, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5470, 5725, 80, 0, 250, 0),
		REG_RULE(5725, 5850, 80, 0, 23, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_CX = {
	.alpha2 = "CX",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_CY = {
	.alpha2 = "CY",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_CZ = {
	.alpha2 = "CZ",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_DE = {
	.alpha2 = "DE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_DK = {
	.alpha2 = "DK",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_DM = {
	.alpha2 = "DM",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_DO = {
	.alpha2 = "DO",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_DZ = {
	.alpha2 = "DZ",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5670, 160, 0, 23, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_EC = {
	.alpha2 = "EC",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 50, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5250, 5350, 80, 0, 125, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 125, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_EE = {
	.alpha2 = "EE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_EG = {
	.alpha2 = "EG",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_ES = {
	.alpha2 = "ES",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_ET = {
	.alpha2 = "ET",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_FI = {
	.alpha2 = "FI",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_FM = {
	.alpha2 = "FM",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_FR = {
	.alpha2 = "FR",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 71000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_GB = {
	.alpha2 = "GB",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5730, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5925, 6425, 160, 0, 250, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 71000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_GD = {
	.alpha2 = "GD",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_GE = {
	.alpha2 = "GE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 18, 0),
		REG_RULE(5250, 5330, 80, 0, 18, 
			NL80211_RRF_DFS | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GF = {
	.alpha2 = "GF",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GH = {
	.alpha2 = "GH",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_GL = {
	.alpha2 = "GL",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GP = {
	.alpha2 = "GP",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GR = {
	.alpha2 = "GR",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_GT = {
	.alpha2 = "GT",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_GU = {
	.alpha2 = "GU",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 20, 0, 17, 0),
		REG_RULE(5250, 5330, 20, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 20, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_GY = {
	.alpha2 = "GY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_HK = {
	.alpha2 = "HK",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 36, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 23, 
			NL80211_RRF_DFS | 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5470, 5730, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5730, 5850, 80, 0, 36, 0),
		REG_RULE(5925, 6425, 160, 0, 14, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_HN = {
	.alpha2 = "HN",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_HR = {
	.alpha2 = "HR",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5350, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_HT = {
	.alpha2 = "HT",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_HU = {
	.alpha2 = "HU",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_ID = {
	.alpha2 = "ID",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 27, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5150, 5350, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5725, 5825, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_IE = {
	.alpha2 = "IE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_IL = {
	.alpha2 = "IL",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 23, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_IN = {
	.alpha2 = "IN",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 30, 0),
		REG_RULE(5250, 5350, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_IR = {
	.alpha2 = "IR",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_IS = {
	.alpha2 = "IS",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_IT = {
	.alpha2 = "IT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_JM = {
	.alpha2 = "JM",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_JO = {
	.alpha2 = "JO",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5735, 5835, 80, 0, 23, 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_JP = {
	.alpha2 = "JP",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(2474, 2494, 20, 0, 20, 
			NL80211_RRF_NO_OFDM | 0),
		REG_RULE(4910, 4990, 40, 0, 23, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5925, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 10, 0),
	},
	.n_reg_rules = 8
};

static const struct ieee80211_regdomain regdom_KE = {
	.alpha2 = "KE",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5490, 5570, 80, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5775, 40, 0, 23, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_KH = {
	.alpha2 = "KH",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_KN = {
	.alpha2 = "KN",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5815, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_KP = {
	.alpha2 = "KP",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 20, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 20, 0),
		REG_RULE(5250, 5330, 20, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5630, 20, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5815, 20, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_KR = {
	.alpha2 = "KR",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 23, 0),
		REG_RULE(5150, 5230, 40, 0, 23, 0),
		REG_RULE(5230, 5250, 20, 0, 17, 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 23, 0),
		REG_RULE(5925, 7125, 160, 0, 15, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 43, 0),
	},
	.n_reg_rules = 8
};

static const struct ieee80211_regdomain regdom_KW = {
	.alpha2 = "KW",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_KY = {
	.alpha2 = "KY",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_KZ = {
	.alpha2 = "KZ",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_LB = {
	.alpha2 = "LB",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_LC = {
	.alpha2 = "LC",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5815, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_LI = {
	.alpha2 = "LI",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_LK = {
	.alpha2 = "LK",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 17, 0),
		REG_RULE(5250, 5330, 20, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 20, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_LS = {
	.alpha2 = "LS",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_LT = {
	.alpha2 = "LT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_LU = {
	.alpha2 = "LU",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_LV = {
	.alpha2 = "LV",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_MA = {
	.alpha2 = "MA",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_MC = {
	.alpha2 = "MC",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MD = {
	.alpha2 = "MD",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_ME = {
	.alpha2 = "ME",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MF = {
	.alpha2 = "MF",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MH = {
	.alpha2 = "MH",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MK = {
	.alpha2 = "MK",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_MN = {
	.alpha2 = "MN",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MO = {
	.alpha2 = "MO",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 23, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MP = {
	.alpha2 = "MP",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MQ = {
	.alpha2 = "MQ",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MR = {
	.alpha2 = "MR",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MT = {
	.alpha2 = "MT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_MU = {
	.alpha2 = "MU",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MV = {
	.alpha2 = "MV",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MW = {
	.alpha2 = "MW",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_MX = {
	.alpha2 = "MX",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_MY = {
	.alpha2 = "MY",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5650, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 24, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_NG = {
	.alpha2 = "NG",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 30, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_NI = {
	.alpha2 = "NI",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_NL = {
	.alpha2 = "NL",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_NO = {
	.alpha2 = "NO",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 71000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_NP = {
	.alpha2 = "NP",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 20, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_NZ = {
	.alpha2 = "NZ",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_OM = {
	.alpha2 = "OM",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PA = {
	.alpha2 = "PA",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 36, 0),
		REG_RULE(5150, 5250, 80, 0, 36, 0),
		REG_RULE(5250, 5350, 80, 0, 30, 0),
		REG_RULE(5470, 5725, 160, 0, 30, 0),
		REG_RULE(5725, 5850, 80, 0, 36, 0),
		REG_RULE(57000, 64000, 2160, 0, 43, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_PE = {
	.alpha2 = "PE",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_PF = {
	.alpha2 = "PF",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PG = {
	.alpha2 = "PG",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_PH = {
	.alpha2 = "PH",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_PK = {
	.alpha2 = "PK",
	.dfs_region = NL80211_DFS_JP,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5150, 5270, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5270, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5610, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5610, 5725, 80, 0, 23, 0),
		REG_RULE(5725, 5875, 80, 0, 30, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_PL = {
	.alpha2 = "PL",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_PM = {
	.alpha2 = "PM",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_PR = {
	.alpha2 = "PR",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_PT = {
	.alpha2 = "PT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_PW = {
	.alpha2 = "PW",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_PY = {
	.alpha2 = "PY",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_QA = {
	.alpha2 = "QA",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 
			NL80211_RRF_NO_OUTDOOR | 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_RE = {
	.alpha2 = "RE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_RO = {
	.alpha2 = "RO",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_RS = {
	.alpha2 = "RS",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_RU = {
	.alpha2 = "RU",
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 100, 0),
		REG_RULE(5150, 5350, 160, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5650, 5850, 160, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5925, 6425, 160, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 
			NL80211_RRF_NO_OUTDOOR | 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_RW = {
	.alpha2 = "RW",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_SA = {
	.alpha2 = "SA",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SE = {
	.alpha2 = "SE",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_SG = {
	.alpha2 = "SG",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 23, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 30, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_SI = {
	.alpha2 = "SI",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_SK = {
	.alpha2 = "SK",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5875, 80, 0, 25, 0),
		REG_RULE(5945, 6425, 160, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 7
};

static const struct ieee80211_regdomain regdom_SN = {
	.alpha2 = "SN",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_SR = {
	.alpha2 = "SR",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SV = {
	.alpha2 = "SV",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 20, 0, 17, 0),
		REG_RULE(5250, 5330, 20, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 20, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_SY = {
	.alpha2 = "SY",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_TC = {
	.alpha2 = "TC",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_TD = {
	.alpha2 = "TD",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_TG = {
	.alpha2 = "TG",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_TH = {
	.alpha2 = "TH",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_TN = {
	.alpha2 = "TN",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_TR = {
	.alpha2 = "TR",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5470, 5725, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_TT = {
	.alpha2 = "TT",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_TW = {
	.alpha2 = "TW",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5350, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5730, 160, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5725, 5850, 80, 0, 30, 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_TZ = {
	.alpha2 = "TZ",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 2
};

static const struct ieee80211_regdomain regdom_UA = {
	.alpha2 = "UA",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2400, 2483, 40, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5150, 5250, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5250, 5350, 80, 0, 20, 
			NL80211_RRF_DFS | 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5470, 5725, 160, 0, 20, 
			NL80211_RRF_DFS | 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5725, 5850, 80, 0, 20, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57000, 66000, 2160, 0, 40, 
			NL80211_RRF_NO_OUTDOOR | 0),
	},
	.n_reg_rules = 6
};

static const struct ieee80211_regdomain regdom_UG = {
	.alpha2 = "UG",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_US = {
	.alpha2 = "US",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(902, 904, 2, 0, 30, 0),
		REG_RULE(904, 920, 16, 0, 30, 0),
		REG_RULE(920, 928, 8, 0, 30, 0),
		REG_RULE(2400, 2472, 40, 0, 30, 0),
		REG_RULE(5150, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5350, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5470, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5730, 5850, 80, 0, 30, 0),
		REG_RULE(5850, 5895, 40, 0, 27, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(5925, 7125, 320, 0, 12, 
			NL80211_RRF_NO_OUTDOOR | 0),
		REG_RULE(57240, 71000, 2160, 0, 40, 0),
	},
	.n_reg_rules = 11
};

static const struct ieee80211_regdomain regdom_UY = {
	.alpha2 = "UY",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_UZ = {
	.alpha2 = "UZ",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 3
};

static const struct ieee80211_regdomain regdom_VC = {
	.alpha2 = "VC",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_VE = {
	.alpha2 = "VE",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 23, 0),
		REG_RULE(5250, 5330, 80, 0, 23, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_VI = {
	.alpha2 = "VI",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2472, 40, 0, 30, 0),
		REG_RULE(5170, 5250, 80, 0, 24, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_VN = {
	.alpha2 = "VN",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_VU = {
	.alpha2 = "VU",
	.dfs_region = NL80211_DFS_FCC,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 17, 0),
		REG_RULE(5250, 5330, 80, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5730, 160, 0, 24, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5735, 5835, 80, 0, 30, 0),
	},
	.n_reg_rules = 5
};

static const struct ieee80211_regdomain regdom_WF = {
	.alpha2 = "WF",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_WS = {
	.alpha2 = "WS",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 40, 0, 20, 0),
		REG_RULE(5250, 5330, 40, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 40, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_YE = {
	.alpha2 = "YE",
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
	},
	.n_reg_rules = 1
};

static const struct ieee80211_regdomain regdom_YT = {
	.alpha2 = "YT",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_ZA = {
	.alpha2 = "ZA",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 30, 0),
	},
	.n_reg_rules = 4
};

static const struct ieee80211_regdomain regdom_ZW = {
	.alpha2 = "ZW",
	.dfs_region = NL80211_DFS_ETSI,
	.reg_rules = {
		REG_RULE(2402, 2482, 40, 0, 20, 0),
		REG_RULE(5170, 5250, 80, 0, 20, 0),
		REG_RULE(5250, 5330, 80, 0, 20, 
			NL80211_RRF_DFS | 0),
		REG_RULE(5490, 5710, 160, 0, 27, 
			NL80211_RRF_DFS | 0),
	},
	.n_reg_rules = 4
};

const struct ieee80211_regdomain *reg_regdb[] = {
	&regdom_00,
	&regdom_AD,
	&regdom_AE,
	&regdom_AF,
	&regdom_AI,
	&regdom_AL,
	&regdom_AM,
	&regdom_AN,
	&regdom_AR,
	&regdom_AS,
	&regdom_AT,
	&regdom_AU,
	&regdom_AW,
	&regdom_AZ,
	&regdom_BA,
	&regdom_BB,
	&regdom_BD,
	&regdom_BE,
	&regdom_BF,
	&regdom_BG,
	&regdom_BH,
	&regdom_BL,
	&regdom_BM,
	&regdom_BN,
	&regdom_BO,
	&regdom_BR,
	&regdom_BS,
	&regdom_BT,
	&regdom_BY,
	&regdom_BZ,
	&regdom_CA,
	&regdom_CF,
	&regdom_CH,
	&regdom_CI,
	&regdom_CL,
	&regdom_CN,
	&regdom_CO,
	&regdom_CR,
	&regdom_CU,
	&regdom_CX,
	&regdom_CY,
	&regdom_CZ,
	&regdom_DE,
	&regdom_DK,
	&regdom_DM,
	&regdom_DO,
	&regdom_DZ,
	&regdom_EC,
	&regdom_EE,
	&regdom_EG,
	&regdom_ES,
	&regdom_ET,
	&regdom_FI,
	&regdom_FM,
	&regdom_FR,
	&regdom_GB,
	&regdom_GD,
	&regdom_GE,
	&regdom_GF,
	&regdom_GH,
	&regdom_GL,
	&regdom_GP,
	&regdom_GR,
	&regdom_GT,
	&regdom_GU,
	&regdom_GY,
	&regdom_HK,
	&regdom_HN,
	&regdom_HR,
	&regdom_HT,
	&regdom_HU,
	&regdom_ID,
	&regdom_IE,
	&regdom_IL,
	&regdom_IN,
	&regdom_IR,
	&regdom_IS,
	&regdom_IT,
	&regdom_JM,
	&regdom_JO,
	&regdom_JP,
	&regdom_KE,
	&regdom_KH,
	&regdom_KN,
	&regdom_KP,
	&regdom_KR,
	&regdom_KW,
	&regdom_KY,
	&regdom_KZ,
	&regdom_LB,
	&regdom_LC,
	&regdom_LI,
	&regdom_LK,
	&regdom_LS,
	&regdom_LT,
	&regdom_LU,
	&regdom_LV,
	&regdom_MA,
	&regdom_MC,
	&regdom_MD,
	&regdom_ME,
	&regdom_MF,
	&regdom_MH,
	&regdom_MK,
	&regdom_MN,
	&regdom_MO,
	&regdom_MP,
	&regdom_MQ,
	&regdom_MR,
	&regdom_MT,
	&regdom_MU,
	&regdom_MV,
	&regdom_MW,
	&regdom_MX,
	&regdom_MY,
	&regdom_NG,
	&regdom_NI,
	&regdom_NL,
	&regdom_NO,
	&regdom_NP,
	&regdom_NZ,
	&regdom_OM,
	&regdom_PA,
	&regdom_PE,
	&regdom_PF,
	&regdom_PG,
	&regdom_PH,
	&regdom_PK,
	&regdom_PL,
	&regdom_PM,
	&regdom_PR,
	&regdom_PT,
	&regdom_PW,
	&regdom_PY,
	&regdom_QA,
	&regdom_RE,
	&regdom_RO,
	&regdom_RS,
	&regdom_RU,
	&regdom_RW,
	&regdom_SA,
	&regdom_SE,
	&regdom_SG,
	&regdom_SI,
	&regdom_SK,
	&regdom_SN,
	&regdom_SR,
	&regdom_SV,
	&regdom_SY,
	&regdom_TC,
	&regdom_TD,
	&regdom_TG,
	&regdom_TH,
	&regdom_TN,
	&regdom_TR,
	&regdom_TT,
	&regdom_TW,
	&regdom_TZ,
	&regdom_UA,
	&regdom_UG,
	&regdom_US,
	&regdom_UY,
	&regdom_UZ,
	&regdom_VC,
	&regdom_VE,
	&regdom_VI,
	&regdom_VN,
	&regdom_VU,
	&regdom_WF,
	&regdom_WS,
	&regdom_YE,
	&regdom_YT,
	&regdom_ZA,
	&regdom_ZW,
};

int reg_regdb_size = ARRAY_SIZE(reg_regdb);
