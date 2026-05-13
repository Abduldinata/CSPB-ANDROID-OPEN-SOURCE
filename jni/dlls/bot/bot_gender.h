#ifndef BOT_GENDER_H
#define BOT_GENDER_H

// CSPB Character Mapping for Bot Balance
// Gender: 0 = Male, 1 = Female

struct BotCharacter
{
    const char *name;
    int skinIndex; // 1-indexed for the menu/engine
    int gender;    // 0: Male, 1: Female
};

// CT Team (Blue)
static BotCharacter g_CT_Characters[] = 
{
    { "Urban", 1, 0 },
    { "Keen Eyes", 2, 1 },
    { "Leopard", 3, 0 },
    { "Hide", 4, 1 },
    { "Judy Chou", 5, 1 }
};

// T Team (Red)
static BotCharacter g_T_Characters[] = 
{
    { "Red Bulls", 1, 0 },
    { "Tarantula", 2, 1 },
    { "D-Fox", 3, 0 },
    { "Viper Red", 4, 1 },
    { "Rica", 5, 1 }
};

#define MAX_BOT_CHARACTERS 5

#endif // BOT_GENDER_H
