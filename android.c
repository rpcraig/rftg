#define LOG_TAG "rftgJNI"
#include "Log.h"
#include "Native.h"
#include "rftg.h"

int display_deck, display_discard, display_pool;
int hand_size, player_us;
int action_max, action_min, action_restrict;
int action_cidx, action_oidx;
int action_payment_which, action_payment_mil;
int *orig_log[MAX_PLAYER];
int *orig_history[MAX_PLAYER];
int restart_loop, num_undo;
displayed hand[MAX_DECK];
options opt;
decisions ai_func;
decisions gui_func;

static JavaVM *cached_jvm;
static jclass Class_C;

void redraw_status(void) {
}

void redraw_phase(void) {
}

void redraw_hand(void) {
}

void redraw_table(void) {
}

void redraw_goal(void) {
}

int gui_choose_search_type(game *g, int who) {
}

int gui_choose_search_keep(game *g, int who, int arg1, int arg2) {
}

int gui_choose_oort_kind(game *g, int who) {
}

void gui_choose_produce(game *g, int who, int cidx[], int oidx[], int num) {
}

void gui_choose_action(game *g, int who, int action[2], int one) {
}

int gui_choose_lucky(game *g, int who) {
}

void gui_choose_takeover_prevent(game *g, int who, int list[], int *num,
                                        int special[], int *num_special) {
}

void gui_choose_consume(game *g, int who, int cidx[], int oidx[], int *num,
                               int *num_special, int optional) {
}

void gui_choose_action_advanced(game *g, int who, int action[2], int one) {
}

void label_set_text(char *buf) {
}

void widget_set_sensitive(bool activate) {
}

void modify_gui(void) {
}

void restore_status_area(int i) {
}

void restore_table_area(int i) {
}

void process_events() {
}

void clear_log() {
}

void message_add(game *g, char *msg) {
}

static void initGame(JNIEnv *env, jclass classz) {
}

static void readCards(JNIEnv *env, jclass clazz) {
}

/*
 * JNI registration.
 */
static JNINativeMethod methods[] = {
  /* name,            signature,       funcPtr */
  { "readCards"       , "()V",         (void*)readCards },
  { "initGame"        , "()V",         (void*)initGame  },
};

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *jvm, void *reserved)
{
     JNIEnv *env;
     jclass cls;
     cached_jvm = jvm;  /* cache the JavaVM pointer */

     if (((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6)) != JNI_OK) {
         return JNI_ERR; /* JNI version not supported */
     }
     cls = (*env)->FindClass(env, "com/example/rftg/MainActivity");
     if (cls == NULL) {
         return JNI_ERR;
     }

     /* Use weak global ref to allow C class to be unloaded */
     Class_C = (*env)->NewWeakGlobalRef(env, cls);
     if (Class_C == NULL) {
         return JNI_ERR;
     }

     (*env)->RegisterNatives(env, cls,
                                 methods, sizeof(methods)/sizeof(methods[0]));

    return JNI_VERSION_1_6;
}

jint JNI_UnLoad(JavaVM* vm, void* reserved) {
	return 1;
}
