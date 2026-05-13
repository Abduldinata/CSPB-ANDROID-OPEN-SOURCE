#pragma once
#include <string.h>

inline bool FileExists(const char* path) {
    int length = 0;
    byte* data = LOAD_FILE_FOR_ME((char*)path, &length);
    if (data) {
        FREE_FILE(data);
        return true;
    }

    return false;
}

inline bool CSPB_ModelPathContains(const char* model, const char* token) {
    return model && token && strstr(model, token) != NULL;
}

inline bool CSPB_IsMeleeModelPath(const char* model) {
    return
        CSPB_ModelPathContains(model, "knife") ||
        CSPB_ModelPathContains(model, "blade") ||
        CSPB_ModelPathContains(model, "sword") ||
        CSPB_ModelPathContains(model, "axe") ||
        CSPB_ModelPathContains(model, "saber") ||
        CSPB_ModelPathContains(model, "dagger") ||
        CSPB_ModelPathContains(model, "katana") ||
        CSPB_ModelPathContains(model, "kukri") ||
        CSPB_ModelPathContains(model, "karambit") ||
        CSPB_ModelPathContains(model, "keris") ||
        CSPB_ModelPathContains(model, "amok") ||
        CSPB_ModelPathContains(model, "fang");
}

inline const char* CSPB_CategorizedModelFallback(const char* model, const char* fallback) {
    if (fallback && FileExists(fallback)) {
        return fallback;
    }

    if (CSPB_ModelPathContains(model, "models/shield/")) {
        if (CSPB_IsMeleeModelPath(model) && FileExists("models/p_m7.mdl")) {
            return "models/p_m7.mdl";
        }

        if (CSPB_ModelPathContains(model, "grenade") && FileExists("models/p_hegrenade.mdl")) {
            return "models/p_hegrenade.mdl";
        }

        if (FileExists("models/p_k5.mdl")) {
            return "models/p_k5.mdl";
        }

        if (FileExists("models/p_usp.mdl")) {
            return "models/p_usp.mdl";
        }
    }

    if (CSPB_ModelPathContains(model, "models/billflx/")) {
        if (CSPB_IsMeleeModelPath(model) && FileExists("models/billflx/v_m7.mdl")) {
            return "models/billflx/v_m7.mdl";
        }

        if (CSPB_ModelPathContains(model, "grenade") && FileExists("models/billflx/v_k400.mdl")) {
            return "models/billflx/v_k400.mdl";
        }

        if (FileExists("models/billflx/v_k5.mdl")) {
            return "models/billflx/v_k5.mdl";
        }
    }

    if (CSPB_ModelPathContains(model, "models/p_")) {
        if (CSPB_IsMeleeModelPath(model) && FileExists("models/p_m7.mdl")) {
            return "models/p_m7.mdl";
        }

        if (CSPB_ModelPathContains(model, "grenade") && FileExists("models/p_hegrenade.mdl")) {
            return "models/p_hegrenade.mdl";
        }

        if (FileExists("models/p_k5.mdl")) {
            return "models/p_k5.mdl";
        }

        if (FileExists("models/p_usp.mdl")) {
            return "models/p_usp.mdl";
        }
    }

    if (CSPB_ModelPathContains(model, "models/w_")) {
        if (CSPB_IsMeleeModelPath(model) && FileExists("models/w_miniaxe.mdl")) {
            return "models/w_miniaxe.mdl";
        }

        if (FileExists("models/w_ak47_fc_bomb.mdl")) {
            return "models/w_ak47_fc_bomb.mdl";
        }

        if (FileExists("models/w_backpack.mdl")) {
            return "models/w_backpack.mdl";
        }
    }

    return fallback ? fallback : model;
}

inline const char* RESOLVE_MODEL_OR_FALLBACK(const char* model, const char* fallback) {
    if (!model || !model[0]) {
        return fallback;
    }

    if (FileExists(model)) {
        return model;
    }

    return CSPB_CategorizedModelFallback(model, fallback);
}

// Safe wrapper for SET_MODEL
inline void SAFE_SET_MODEL(edict_t* pent, const char* model) {
    SET_MODEL(pent, RESOLVE_MODEL_OR_FALLBACK(model, "models/w_backpack.mdl"));
}

// Safe wrapper for PRECACHE_MODEL
inline void SAFE_PRECACHE_MODEL(const char* model) {
    PRECACHE_MODEL(RESOLVE_MODEL_OR_FALLBACK(model, "models/w_backpack.mdl"));
}

// CSPB temporary world-model policy:
// - keep the preferred future asset path as the first argument
// - while CSPB still lacks a dedicated W model, use an existing CSPB placeholder
// - firearm-like items use a weapon-shaped placeholder instead of the generic backpack
inline const char* RESOLVE_CSPB_FIREARM_WORLD_MODEL(const char* preferredModel) {
    return RESOLVE_MODEL_OR_FALLBACK(preferredModel, "models/w_ak47_fc_bomb.mdl");
}

// CSPB temporary melee world-model policy:
// - keep the preferred future asset path as the first argument
// - while CSPB still lacks a dedicated W model, use the existing mini axe world model
inline const char* RESOLVE_CSPB_MELEE_WORLD_MODEL(const char* preferredModel) {
    return RESOLVE_MODEL_OR_FALLBACK(preferredModel, "models/w_miniaxe.mdl");
}
