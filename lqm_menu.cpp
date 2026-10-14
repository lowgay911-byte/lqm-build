// lqm_menu.cpp
// Build via GitHub Actions

#define LOG_TAG "LQM_MENU"
#include <android/log.h>
#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// EGL / GLES3 minimal
typedef void* EGLDisplay;
typedef void* EGLSurface;
typedef int   EGLBoolean;

extern "C" {
    EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surf);
}

typedef unsigned int GLenum;
typedef int          GLint;
typedef float        GLfloat;

#define GL_DEPTH_TEST    0x0B71
#define GL_CULL_FACE     0x0B44
#define GL_VIEWPORT      0x0BA2
#define GL_PROJECTION    0x1701
#define GL_MODELVIEW     0x1700
#define GL_LINE_LOOP     0x0002
#define GL_LINES         0x0001
#define GL_QUADS         0x0007

extern "C" {
    void glGetIntegerv(GLenum pname, GLint* data);
    void glDisable(GLenum cap);
    void glEnable(GLenum cap);
    void glLineWidth(GLfloat w);
    void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
    void glBegin(GLenum mode);
    void glEnd(void);
    void glVertex2f(GLfloat x, GLfloat y);
    void glMatrixMode(GLenum mode);
    void glPushMatrix(void);
    void glPopMatrix(void);
    void glLoadIdentity(void);
    void glOrthof(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
}

// IL2CPP
using il2cpp_domain_get_t               = void* (*)();
using il2cpp_domain_assembly_open_t     = void* (*)(void*, const char*);
using il2cpp_assembly_get_image_t       = void* (*)(void*);
using il2cpp_class_from_name_t          = void* (*)(void*, const char*, const char*);
using il2cpp_class_get_method_from_name = void* (*)(void*, const char*, int);
using il2cpp_field_get_offset_t         = size_t (*)(void*);
using il2cpp_class_get_field_from_name  = void* (*)(void*, const char*);
using il2cpp_object_new_t               = void* (*)(void*);
using il2cpp_runtime_invoke_t           = void* (*)(void*, void*, void**, void**);
using il2cpp_thread_attach_t            = void* (*)(void*);

struct IL2CPP {
    void* handle = nullptr;
    il2cpp_domain_get_t               domain_get               = nullptr;
    il2cpp_domain_assembly_open_t     domain_assembly_open     = nullptr;
    il2cpp_assembly_get_image_t       assembly_get_image       = nullptr;
    il2cpp_class_from_name_t          class_from_name          = nullptr;
    il2cpp_class_get_method_from_name class_get_method_from_name = nullptr;
    il2cpp_field_get_offset_t         field_get_offset         = nullptr;
    il2cpp_class_get_field_from_name  class_get_field_from_name = nullptr;
    il2cpp_object_new_t               object_new               = nullptr;
    il2cpp_runtime_invoke_t           runtime_invoke           = nullptr;
    il2cpp_thread_attach_t            thread_attach            = nullptr;
    void* domain = nullptr;

    bool Init() {
        handle = dlopen("libil2cpp.so", RTLD_NOW);
        if (!handle) { LOGE("libil2cpp.so not loaded"); return false; }

        #define RESOLVE_MEMBER(member, symbol) \
            member = (decltype(member))dlsym(handle, symbol); \
            if (!member) { LOGE("missing " symbol); return false; }

        RESOLVE_MEMBER(domain_get,                "il2cpp_domain_get")
        RESOLVE_MEMBER(domain_assembly_open,      "il2cpp_domain_assembly_open")
        RESOLVE_MEMBER(assembly_get_image,        "il2cpp_assembly_get_image")
        RESOLVE_MEMBER(class_from_name,           "il2cpp_class_from_name")
        RESOLVE_MEMBER(class_get_method_from_name,"il2cpp_class_get_method_from_name")
        RESOLVE_MEMBER(field_get_offset,          "il2cpp_field_get_offset")
        RESOLVE_MEMBER(class_get_field_from_name, "il2cpp_class_get_field_from_name")
        RESOLVE_MEMBER(object_new,                "il2cpp_object_new")
        RESOLVE_MEMBER(runtime_invoke,            "il2cpp_runtime_invoke")
        RESOLVE_MEMBER(thread_attach,             "il2cpp_thread_attach")

        #undef RESOLVE_MEMBER

        domain = domain_get();
        if (!domain) { LOGE("no domain"); return false; }
        thread_attach(domain);
        LOGI("IL2CPP ready");
        return true;
    }

    void* Image(const char* asmName) {
        void* asm_ = domain_assembly_open(domain, asmName);
        if (!asm_) return nullptr;
        return assembly_get_image(asm_);
    }
    void* Klass(const char* ns, const char* name, void* image) {
        return class_from_name(image, ns, name);
    }
    void* Method(void* klass, const char* name, int argc) {
        return class_get_method_from_name(klass, name, argc);
    }
    size_t FieldOffset(void* klass, const char* name) {
        void* f = class_get_field_from_name(klass, name);
        return f ? field_get_offset(f) : 0;
    }
};

static IL2CPP il2;

struct GameRefs {
    void* image = nullptr;
    void* klass_HeroBase    = nullptr;
    void* klass_PlayerCtrl  = nullptr;
    void* klass_MinionCtrl  = nullptr;
    void* klass_CameraCtrl  = nullptr;
    void* klass_GameManager = nullptr;
    void* klass_SkillComp   = nullptr;
    void* klass_AttackComp  = nullptr;
    void* klass_ViewComp    = nullptr;
    void* klass_MapCtrl     = nullptr;
    void* klass_MatchInfo   = nullptr;

    size_t off_Hero_HP          = 0x000;
    size_t off_Hero_MaxHP       = 0x000;
    size_t off_Hero_Mana        = 0x000;
    size_t off_Hero_MaxMana     = 0x000;
    size_t off_Hero_Pos         = 0x000;
    size_t off_Hero_Team        = 0x000;
    size_t off_Hero_IsLocal     = 0x000;
    size_t off_Hero_IsDead      = 0x000;
    size_t off_Hero_Level       = 0x000;
    size_t off_Hero_Gold        = 0x000;
    size_t off_Hero_AttackRange = 0x000;
    size_t off_Hero_AtkSpeed    = 0x000;
    size_t off_Hero_MoveSpeed   = 0x000;
    size_t off_Hero_CoolDown    = 0x000;

    bool Resolve() {
        image = il2.Image("Assembly-CSharp.dll");
        if (!image) { LOGE("no Assembly-CSharp"); return false; }

        klass_HeroBase    = il2.Klass("", "HeroBase",         image);
        klass_PlayerCtrl  = il2.Klass("", "PlayerController", image);
        klass_MinionCtrl  = il2.Klass("", "MinionController", image);
        klass_CameraCtrl  = il2.Klass("", "CameraController", image);
        klass_GameManager = il2.Klass("", "GameManager",      image);
        klass_SkillComp   = il2.Klass("", "SkillComponent",   image);
        klass_AttackComp  = il2.Klass("", "AttackComponent",  image);
        klass_ViewComp    = il2.Klass("", "ViewComponent",    image);
        klass_MapCtrl     = il2.Klass("", "MapController",    image);
        klass_MatchInfo   = il2.Klass("", "MatchInfo",        image);

        if (!klass_HeroBase || !klass_GameManager) {
            LOGE("class resolve failed");
            return false;
        }
        return true;
    }
};

static GameRefs refs;

struct LiveHero {
    void* obj;
    uintptr_t addr;
    float hp, maxHp, mana, maxMana;
    int   team, level;
    bool  isLocal, isDead;
    float x, y, z;
    float attackRange, atkSpeed, moveSpeed;
};

static std::mutex           g_Lock;
static std::vector<LiveHero> g_Heroes;
static LiveHero             g_Local;
static bool                 g_LocalValid = false;

template<typename T>
static T ReadField(void* obj, size_t offset) {
    if (!obj) return T{};
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset);
}
template<typename T>
static void WriteField(void* obj, size_t offset, T value) {
    if (!obj) return;
    *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset) = value;
}

struct Il2CppArray {
    void* klass;
    void* monitor;
    void* bounds;
    uintptr_t max_length;
};
struct Il2CppList {
    void* klass;
    void* monitor;
    Il2CppArray* items;
    int size;
};

static void CollectHeroes(void* gameManager) {
    static size_t off_GM_HeroList  = 0x0;
    static size_t off_GM_LocalHero = 0x0;

    if (!gameManager) return;
    auto* list = ReadField<Il2CppList*>(gameManager, off_GM_HeroList);
    if (!list || !list->items) return;

    std::lock_guard<std::mutex> lk(g_Lock);
    g_Heroes.clear();

    uintptr_t elemsBase = reinterpret_cast<uintptr_t>(list->items) + sizeof(Il2CppArray);
    for (int i = 0; i < list->size && i < 64; ++i) {
        void* hero = *reinterpret_cast<void**>(elemsBase + i * sizeof(void*));
        if (!hero) continue;

        LiveHero h{};
        h.obj         = hero;
        h.addr        = reinterpret_cast<uintptr_t>(hero);
        h.hp          = ReadField<float>(hero, refs.off_Hero_HP);
        h.maxHp       = ReadField<float>(hero, refs.off_Hero_MaxHP);
        h.mana        = ReadField<float>(hero, refs.off_Hero_Mana);
        h.maxMana     = ReadField<float>(hero, refs.off_Hero_MaxMana);
        h.team        = ReadField<int>(hero,   refs.off_Hero_Team);
        h.level       = ReadField<int>(hero,   refs.off_Hero_Level);
        h.isDead      = ReadField<bool>(hero,  refs.off_Hero_IsDead);
        h.attackRange = ReadField<float>(hero, refs.off_Hero_AttackRange);
        h.atkSpeed    = ReadField<float>(hero, refs.off_Hero_AtkSpeed);
        h.moveSpeed   = ReadField<float>(hero, refs.off_Hero_MoveSpeed);

        uintptr_t posAddr = h.addr + refs.off_Hero_Pos;
        h.x = *reinterpret_cast<float*>(posAddr + 0);
        h.y = *reinterpret_cast<float*>(posAddr + 4);
        h.z = *reinterpret_cast<float*>(posAddr + 8);

        void* localHero = ReadField<void*>(gameManager, off_GM_LocalHero);
        h.isLocal = (hero == localHero);

        g_Heroes.push_back(h);
        if (h.isLocal) { g_Local = h; g_LocalValid = true; }
    }
}

struct Menu {
    bool  godMode           = false;
    bool  infiniteMana      = false;
    bool  noCooldown        = false;
    bool  damageMult        = false;
    float damageMultVal     = 10.f;
    bool  oneHitKill        = false;
    bool  instantKill       = false;

    bool  speedHack         = false;
    float speedMult         = 3.f;
    bool  teleport          = false;
    float tpX = 0, tpY = 0, tpZ = 0;
    bool  noclip            = false;
    bool  flyHack           = false;

    bool  esp               = true;
    bool  espBox            = true;
    bool  espHP             = true;
    bool  espName           = true;
    bool  espLine           = false;
    bool  espDistance       = true;
    bool  showMinions       = false;
    bool  showJungle        = false;
    bool  showWards         = false;
    bool  showEnemySkills   = false;
    bool  showRecall        = false;
    bool  mapHack           = true;
    bool  revealFog         = true;

    bool  infiniteGold      = false;
    bool  instantBuy        = false;
    bool  freeShop          = false;
    bool  maxLevel          = false;
    bool  instantRespawn    = false;

    bool  aimbot            = false;
    float aimFov            = 45.f;
    bool  aimOnSkill        = true;
    bool  skillAim          = false;
    bool  autoCombo         = false;

    bool  autoFarm          = false;
    bool  autoLastHit       = false;
    bool  autoDodge         = false;
    bool  autoSkill         = false;
    bool  autoRecall        = false;

    bool  revealEnemies     = true;
    bool  revealAll         = true;

    bool  timeScale         = false;
    float timeScaleVal      = 1.f;
    bool  freezeEnemies     = false;
    bool  freezeMinions     = false;
    bool  freezeTowers      = false;

    bool  zoomHack          = false;
    float zoomVal           = 20.f;
    bool  freeCamera        = false;
    float camX = 0, camY = 0, camZ = 0;
};
static Menu g_Menu;

static void* GetGameManager();

static void ApplyFeatures() {
    std::lock_guard<std::mutex> lk(g_Lock);
    if (!g_LocalValid) return;

    void* me = g_Local.obj;

    if (g_Menu.godMode) {
        WriteField<float>(me, refs.off_Hero_HP,    g_Local.maxHp);
        WriteField<float>(me, refs.off_Hero_MaxHP, g_Local.maxHp);
    }
    if (g_Menu.infiniteMana) {
        WriteField<float>(me, refs.off_Hero_Mana, g_Local.maxMana);
    }
    if (g_Menu.noCooldown) {
        WriteField<float>(me, refs.off_Hero_CoolDown, 0.f);
    }
    if (g_Menu.speedHack) {
        WriteField<float>(me, refs.off_Hero_MoveSpeed, g_Local.moveSpeed * g_Menu.speedMult);
    }
    if (g_Menu.maxLevel) {
        WriteField<int>(me, refs.off_Hero_Level, 15);
    }
    if (g_Menu.instantRespawn) {
        WriteField<bool>(me, refs.off_Hero_IsDead, false);
    }
    if (g_Menu.teleport) {
        uintptr_t posAddr = g_Local.addr + refs.off_Hero_Pos;
        *reinterpret_cast<float*>(posAddr + 0) = g_Menu.tpX;
        *reinterpret_cast<float*>(posAddr + 4) = g_Menu.tpY;
        *reinterpret_cast<float*>(posAddr + 8) = g_Menu.tpZ;
    }

    for (auto& h : g_Heroes) {
        if (h.isLocal) continue;
        if (h.team == g_Local.team) continue;

        if (g_Menu.oneHitKill || g_Menu.instantKill) {
            WriteField<float>(h.obj, refs.off_Hero_HP, 0.f);
            WriteField<bool>(h.obj,  refs.off_Hero_IsDead, true);
        }
        if (g_Menu.freezeEnemies) {
            WriteField<float>(h.obj, refs.off_Hero_MoveSpeed, 0.f);
            WriteField<float>(h.obj, refs.off_Hero_AtkSpeed,  0.f);
        }
    }
}

extern "C" void DobbyHook(void* target, void* replace, void** origin);

using UpdateFn = void(*)(void*);
static UpdateFn orig_HeroUpdate = nullptr;

static void Hook_HeroUpdate(void* self) {
    static std::chrono::steady_clock::time_point lastFrame;
    auto now = std::chrono::steady_clock::now();
    bool newFrame = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrame).count() > 4000;

    if (newFrame) {
        lastFrame = now;
        void* gm = GetGameManager();
        if (gm) {
            CollectHeroes(gm);
            ApplyFeatures();
        }
    }

    if (orig_HeroUpdate) orig_HeroUpdate(self);
}

static void* GetGameManager() {
    static void* cached = nullptr;
    if (cached) return cached;
    return cached;
}

using eglSwapBuffers_t = EGLBoolean(*)(EGLDisplay, EGLSurface);
static eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

struct Vec2 { float x, y; };
static int g_ScreenW = 0, g_ScreenH = 0;

static Vec2 WorldToScreen(float wx, float wy, float wz,
                          const float* view, const float* proj,
                          int sw, int sh) {
    float vp[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            float s = 0;
            for (int k = 0; k < 4; ++k)
                s += proj[k*4 + i] * view[j*4 + k];
            vp[j*4 + i] = s;
        }
    float cx = vp[0]*wx + vp[4]*wy + vp[8]*wz  + vp[12];
    float cy = vp[1]*wx + vp[5]*wy + vp[9]*wz  + vp[13];
    float cw = vp[3]*wx + vp[7]*wy + vp[11]*wz + vp[15];
    if (cw < 0.01f) return { -1, -1 };
    return { (sw * 0.5f) * (1.f + cx/cw), (sh * 0.5f) * (1.f - cy/cw) };
}

static void DrawESP_GLES() {
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    g_ScreenW = vp[2];
    g_ScreenH = vp[3];

    static float view[16] = {0};
    static float proj[16] = {0};

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrthof(0, g_ScreenW, g_ScreenH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    std::lock_guard<std::mutex> lk(g_Lock);
    int localTeam = g_LocalValid ? g_Local.team : -1;

    for (auto& h : g_Heroes) {
        if (h.isLocal) continue;
        if (h.team == localTeam) continue;
        if (h.isDead) continue;

        Vec2 head = WorldToScreen(h.x, h.y, h.z + 200.f, view, proj, g_ScreenW, g_ScreenH);
        Vec2 feet = WorldToScreen(h.x, h.y, h.z,         view, proj, g_ScreenW, g_ScreenH);
        if (head.x < 0 || feet.x < 0) continue;

        float hgt = feet.y - head.y;
        float wid = hgt * 0.5f;

        glLineWidth(2.f);
        glColor4f(1.f, 0.2f, 0.2f, 1.f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(head.x - wid/2, head.y);
            glVertex2f(head.x + wid/2, head.y);
            glVertex2f(head.x + wid/2, feet.y);
            glVertex2f(head.x - wid/2, feet.y);
        glEnd();

        if (g_Menu.espHP) {
            float frac = (h.maxHp > 0) ? (h.hp / h.maxHp) : 0.f;
            float barX = head.x - wid/2 - 6.f;
            glColor4f(0.f, 1.f, 0.f, 1.f);
            glBegin(GL_QUADS);
                glVertex2f(barX, feet.y - hgt*frac);
                glVertex2f(barX + 4.f, feet.y - hgt*frac);
                glVertex2f(barX + 4.f, feet.y);
                glVertex2f(barX, feet.y);
            glEnd();
        }
        if (g_Menu.espLine) {
            glColor4f(1.f, 0.2f, 0.2f, 0.6f);
            glBegin(GL_LINES);
                glVertex2f(g_ScreenW * 0.5f, (float)g_ScreenH);
                glVertex2f(feet.x, feet.y);
            glEnd();
        }
    }

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

static EGLBoolean Hook_eglSwapBuffers(EGLDisplay d, EGLSurface s) {
    if (g_Menu.esp) DrawESP_GLES();
    return orig_eglSwapBuffers(d, s);
}

static bool  g_MenuOpen = false;
static int   g_TouchCount = 0;

using getTouchCount_t = int(*)(void*);
static getTouchCount_t orig_getTouchCount = nullptr;
static int Hook_getTouchCount(void* self) {
    int n = orig_getTouchCount ? orig_getTouchCount(self) : 0;
    if (n == 3 && g_TouchCount != 3) g_MenuOpen = !g_MenuOpen;
    g_TouchCount = n;
    return n;
}

static void* WorkerThread(void*) {
    for (int i = 0; i < 200; ++i) {
        if (dlopen("libil2cpp.so", RTLD_NOW)) break;
        usleep(100000);
    }
    if (!il2.Init())    return nullptr;
    if (!refs.Resolve()) return nullptr;

    void* updateMethod = il2.Method(refs.klass_HeroBase, "Update", 0);
    if (updateMethod) {
        void* nativePtr = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(updateMethod) + 0x0);
        if (nativePtr) DobbyHook(nativePtr, (void*)Hook_HeroUpdate, (void**)&orig_HeroUpdate);
    }

    void* egl = dlopen("libEGL.so", RTLD_NOW);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) DobbyHook(swap, (void*)Hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    }

    void* inputMethod = il2.Method(
        il2.Klass("", "Input", refs.image), "get_touchCount", 0);
    if (inputMethod) {
        void* nativePtr = *reinterpret_cast<void**>(
            reinterpret_cast<uintptr_t>(inputMethod) + 0x0);
        if (nativePtr) DobbyHook(nativePtr, (void*)Hook_getTouchCount, (void**)&orig_getTouchCount);
    }

    LOGI("LQM menu online");
    return nullptr;
}

__attribute__((constructor))
static void OnLoad() {
    LOGI("liblqm_menu loaded");
    pthread_t t;
    pthread_create(&t, nullptr, WorkerThread, nullptr);
    pthread_detach(t);
}

extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setGodMode(JNIEnv*, jclass, jboolean v)   { g_Menu.godMode = v; }
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setOneHitKill(JNIEnv*, jclass, jboolean v){ g_Menu.oneHitKill = v; }
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setSpeedHack(JNIEnv*, jclass, jboolean v) { g_Menu.speedHack = v; }
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setTeleport(JNIEnv*, jclass, jfloat x, jfloat y, jfloat z) {
    g_Menu.teleport = true; g_Menu.tpX = x; g_Menu.tpY = y; g_Menu.tpZ = z;
}
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setEsp(JNIEnv*, jclass, jboolean v)       { g_Menu.esp = v; }
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setMapHack(JNIEnv*, jclass, jboolean v)   { g_Menu.mapHack = v; }
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setFreeze(JNIEnv*, jclass, jboolean v)    { g_Menu.freezeEnemies = v; }
extern "C" JNIEXPORT void JNICALL
Java_com_lqm_menu_Native_setZoom(JNIEnv*, jclass, jboolean v, jfloat z) {
    g_Menu.zoomHack = v; g_Menu.zoomVal = z;
}