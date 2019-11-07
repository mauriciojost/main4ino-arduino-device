#ifndef MODULE_INC
#define MODULE_INC

#include <log4ino/Log.h>
#include <main4ino/Actor.h>
#include <main4ino/Array.h>
#include <main4ino/Clock.h>
#include <main4ino/ClockSync.h>
#include <main4ino/PropSync.h>
#include <main4ino/SerBot.h>
#include <main4ino/Table.h>
#include <mod4ino/MsgClearMode.h>
#include <mod4ino/Settings.h>

#define CLASS_MODULE "MD"

#define COMMAND_MAX_LENGTH 128

#define PERIOD_CONFIGURE_MSEC 1000

#define HELP_COMMAND_CLI                                                                                                                   \
  "\n  MODULE HELP"                                                                                                                        \
  "\n  int             : interrupt current ongoing action"                                                                                 \
  "\n  mode [run,conf] : get or set the mode"                                                                                              \
  "\n  info            : show info about the device"                                                                                       \
  "\n  test            : test the architecture/hardware"                                                                                   \
  "\n  update ...      : update the firmware with the given target version"                                                                \
  "\n  wifi            : init steady wifi"                                                                                                 \
  "\n  wifistop        : stop steady wifi"                                                                                                 \
  "\n  get             : display actors properties"                                                                                        \
  "\n  get ...         : display actor <actor> properties"                                                                                 \
  "\n  set ...         : set an actor property (example: 'set body msg0 HELLO')"                                                           \
  "\n  logl [...]      : get / change log level to <x> (0 is more verbose, to 3 least verbose)"                                            \
  "\n  clear           : clear device (filesystem, crashes stacktrace, etc.)"                                                              \
  "\n  actall          : all act"                                                                                                          \
  "\n  touchall        : mark actors as 'changed' to force synchronization with the server"                                                \
  "\n  actone ...      : make actor <x> act"                                                                                               \
  "\n  wifissid ...    : set wifi ssid"                                                                                                    \
  "\n  wifipass ...    : set wifi pass"                                                                                                    \
  "\n  wifissidb ...   : set wifi ssid (backup net)"                                                                                       \
  "\n  wifipassb ...   : set wifi pass (backup net)"                                                                                       \
  "\n  cat ...         : show content of a file (only if in insecure mode)"                                                                \
  "\n  load            : load properties in persistent fs (mainly for credentials)"                                                        \
  "\n  store           : save properties in persistent fs (mainly for credentials)"                                                        \
  "\n  save ...        : save a file <f> with content <y> in persistent fs (mainly for tuning) "                                           \
  "\n  help            : show this help"                                                                                                   \
  "\n  (all messages are shown as info log level)"                                                                                         \
  "\n"

enum CmdExecStatus { NotFound = 0, InvalidArgs, Executed, ExecutedInterrupt, CmdFailed };
#define CMD_EXEC_STATUS(s)                                                                                                                 \
  (s == NotFound                                                                                                                           \
       ? "Not found"                                                                                                                       \
       : (s == InvalidArgs                                                                                                                 \
              ? "Invalid args"                                                                                                             \
              : (s == Executed ? "Executed" : (s == ExecutedInterrupt ? "Executed w/int" : (s == CmdFailed ? "Failed" : "Unknown")))))

enum ModuleStartupPropertiesCode {
  ModuleStartupPropertiesCodeSuccess = 0,
  ModuleStartupPropertiesCodeSkipped,
  ModuleStartupPropertiesCodePropertiesSyncFailure,
  ModuleStartupPropertiesCodeClockSyncFailure,
  ModuleStartupPropertiesCodeDelimiter
};

/**
 * This class represents the integration of all components (LCD, buttons, buzzer, etc).
 */
class Module {

private:
  PropSync *propSync;
  ClockSync *clockSync;
  Array<Actor *> *actors;
  Clock *clock;
  Settings *settings;
  SerBot *bot;
  const char *description;

  // Initialization of wifi.
  bool (*initWifi)();

  // Stop wifi.
  void (*stopWifi)();

  // Clear device function (remove filesystem, stacktraces, logs, etc...).
  void (*clearDevice)();

  // Sleep function (must be interruptable, return true if interrupted).
  bool (*sleepInterruptable)(time_t cycleBegin, time_t periodSec);

  // Deep sleep function not interruptable (after sleep it resets)
  void (*deepSleepNotInterruptable)(time_t cycleBegin, time_t periodSec);

  // Run one cycle for the architecture (configure mode).
  void (*configureModeArchitecture)();

  // Run one cycle for the architecture (run mode).
  void (*runModeArchitecture)();

  // Function that executes a command from the underlying architecture point of view.
  CmdExecStatus (*commandArchitecture)(const char *cmd);

  // File read function.
  bool (*fileRead)(const char *fname, Buffer *content);

  // File write function.
  bool (*fileWrite)(const char *fname, const char *content);

  // Information display function (version, general status, battery, etc.).
  void (*info)();

  // HW test.
  void (*test)();

  // Firmware update.
  void (*update)(const char *targetVersion, const char *currentVersion);

  // Retrieve the login for the main4ino API.
  const char *(*apiDeviceLogin)();

  // Retrieve the password for the main4ino API.
  const char *(*apiDevicePass)();

  // Regular HTTP POST.
  int (*httpPost)(const char *url, const char *body, ParamStream *response, Table *headers);

  // Regular HTTP GET.
  int (*httpGet)(const char *url, ParamStream *response, Table *headers);

  // Defines if this module is aimed at a single execution (some devices need this, as deep sleep modes equal full restarts).
  bool (*oneRunMode)();

  /**
   * Core of mod4ino
   *
   * Module that provides:
   * - time keeping and sync services (via Clock and ClockSync classes from main4ino)
   * - property synchronization services (via PropSync class from main4ino)
   *
   * Use as follows:
   *
   *   Module* module = new Module();
   *   module->getActors()->add(n, (Actor *)actor1, ...);
   *   module->setup(...);
   *   ModuleStartupPropertiesCode s = module->startupProperties(...);
   *   if (s == ModuleStartupPropertiesCodeSuccess) {
   *     while (true) {
   *       module->loop();
   *     }
   *   }
   *
   */
public:
  Module() {
    actors = new Array<Actor *>;

    settings = new Settings("settings");
    propSync = new PropSync("propsync");
    clockSync = new ClockSync("clocksync");
    clock = new Clock("clock");

    actors->add(4, (Actor *)settings, (Actor *)propSync, (Actor *)clockSync, (Actor *)clock);

    bot = new SerBot(clock, actors);

    initWifi = NULL;
    stopWifi = NULL;
    clearDevice = NULL;
    httpGet = NULL;
    httpPost = NULL;
    sleepInterruptable = NULL;
    deepSleepNotInterruptable = NULL;
    configureModeArchitecture = NULL;
    runModeArchitecture = NULL;
    commandArchitecture = NULL;
    fileRead = NULL;
    fileWrite = NULL;
    info = NULL;
    test = NULL;
    update = NULL;
    apiDeviceLogin = NULL;
    apiDevicePass = NULL;
    oneRunMode = NULL;

    description = NULL;
  }

public:
  /**
   * Setup this module and the core components and the architecture.
   *
   * The Bot mode set will be the one returned by setupArchitecture().
   */
  void setup(BotMode (*setupArchitecture)(),
             bool (*initWifiFunc)(),
             void (*stopWifiFunc)(),
             int (*httpPostFunc)(const char *url, const char *body, ParamStream *response, Table *headers),
             int (*httpGetFunc)(const char *url, ParamStream *response, Table *headers),
             void (*clearDeviceFunc)(),
             bool (*fileReadFunc)(const char *fname, Buffer *content),
             bool (*fileWriteFunc)(const char *fname, const char *content),
             bool (*sleepInterruptableFunc)(time_t cycleBegin, time_t periodSec),
             void (*deepSleepNotInterruptableFunc)(time_t cycleBegin, time_t periodSec),
             void (*configureModeArchitectureFunc)(),
             void (*runModeArchitectureFunc)(),
             CmdExecStatus (*commandArchitectureFunc)(const char *cmd),
             void (*infoFunc)(),
             void (*updateFunc)(const char *targetVersion, const char *currentVersion),
             void (*testFunc)(),
             const char *(*apiDeviceLoginFunc)(),
             const char *(*apiDevicePassFunc)(),
             bool (*oneRunModeFunc)()) {

    // Unstable situation from now until the end of the function
    //
    // Actors are being initialized. When they act, they use callback functions that
    // may trigger low level calls (like IO, LCD, etc) which are not set up yet.
    // As a good practice, actors should NOT do any low level calls until they are executed the method act().
    // Setup of low level calls happens in setupArchitecture().
    // After that, actors will be eventually asked for acting.

    initWifi = initWifiFunc;
    stopWifi = stopWifiFunc;
    clearDevice = clearDeviceFunc;
    sleepInterruptable = sleepInterruptableFunc;
    deepSleepNotInterruptable = deepSleepNotInterruptableFunc;
    configureModeArchitecture = configureModeArchitectureFunc;
    runModeArchitecture = runModeArchitectureFunc;
    commandArchitecture = commandArchitectureFunc;
    fileRead = fileReadFunc;
    fileWrite = fileWriteFunc;
    info = infoFunc;
    test = testFunc;
    update = updateFunc;
    apiDeviceLogin = apiDeviceLoginFunc;
    apiDevicePass = apiDevicePassFunc;
    httpGet = httpGetFunc;
    httpPost = httpPostFunc;
    oneRunMode = oneRunModeFunc;

    propSync->setup(bot, initWifi, httpGet, httpPost, fileRead, fileWrite);
    clockSync->setup(bot->getClock(), initWifi, httpGet);

    BotMode mode = setupArchitecture(); // module objects initialized, architecture can be initialized now

    getBot()->setMode(mode);
  }

public:
  void setDescription(const char *d) {
    // Example:
    // {"version":"1.0.0","json":[{"patterns": ["^.*.freq$"], "descriptions: ["Description here."],"examples": ["value -> Explanation"]}],
    description = d;
  }

  /**
   * Start up all module's properties
   *
   * Retrieve credentials and other properties from FS and server and report actual ones.
   *
   * 1. Load properties from the file system (general properties/credentials not related to the framework).
   * 0. Load properties from the file system (credentials related to the framework).
   * 2. Pull/push properties from/to the server
   * 3. Set time of actors with last known time
   * 4. Find out real current time
   * 5. Return success if properties and clock sync went well
   *
   */
public:
  ModuleStartupPropertiesCode startupProperties() {

    if (getBot()->getMode() != RunMode) {
      return ModuleStartupPropertiesCodeSkipped;
    }

    log(CLASS_MODULE, Info, "# Loading general properties/creds stored in FS...");
    getPropSync()->fsLoadActorsProps();

    log(CLASS_MODULE, Info, "# Loading main4ino properties/creds stored in FS...");
    getPropSync()->setLoginPass(apiDeviceLogin(), apiDevicePass()); // may override credentials loaded in steps above
    getClockSync()->setLoginPass(apiDeviceLogin(), apiDevicePass());

    log(CLASS_MODULE, Info, "# Syncing actors with main4ino server...");
    bool oneRun = oneRunModeSafe();
    PropSyncStatusCode serSyncd = PropSyncStatusCodeUnknown;
    if (oneRun) {
      log(CLASS_MODULE, Info, "Only pull...");
      serSyncd = getPropSync()->pullActors(); // only pull, push is postponed
    } else {
      log(CLASS_MODULE, Info, "Pull and push...");
      serSyncd = getPropSync()->pullPushActors(false); // sync properties from the server
    }

    if (getPropSync()->isFailure(serSyncd)) {
      stopWifi();
      return ModuleStartupPropertiesCodePropertiesSyncFailure;
    }

    time_t leftTime = getBot()->getClock()->currentTime();

    Buffer timeAux(19);
    log(CLASS_MODULE, Info, "# Previous actors' times: %s...", Timing::humanize(leftTime, &timeAux));
    getBot()->setActorsTime(leftTime);

    log(CLASS_MODULE, Info, "# Syncing clock...");
    // sync real date / time on clock, block if a single run is requested
    bool freezeTime = oneRun;
    bool clockSyncd = getClockSync()->syncClock(freezeTime, DEFAULT_CLOCK_SYNC_ATTEMPTS);
    log(CLASS_MODULE, Info, "# Current time: %s", Timing::humanize(getBot()->getClock()->currentTime(), &timeAux));

    if (description != NULL) {
      log(CLASS_MODULE, Info, "# Sending description...");
      getPropSync()->pushDescription(description);
    } else {
      log(CLASS_MODULE, Info, "# No description to send...");
    }

    if (!clockSyncd) {
      stopWifi();
      return ModuleStartupPropertiesCodeClockSyncFailure;
    }

    stopWifi();
    return ModuleStartupPropertiesCodeSuccess;
  }

  /**
   * Handle a user command.
   * If no command maches, commandArchitecture will be used as fallback.
   */
public:
  CmdExecStatus command(const char *cmd) {

    Buffer b(cmd);
    log(CLASS_MODULE, Info, "\n> %s\n", b.getBuffer());

    if (b.getLength() == 0) {
      return NotFound;
    }

    char *c = strtok(b.getUnsafeBuffer(), " ");

    if (strcmp("set", c) == 0) {
      const char *actor = strtok(NULL, " ");
      const char *prop = strtok(NULL, " ");
      const char *v = strtok(NULL, " ");
      if (actor == NULL || prop == NULL || v == NULL) {
        logRawUser("Arguments needed:\n  set <actor> <prop> <value>");
        return InvalidArgs;
      }
      log(CLASS_MODULE, Info, "-> Set %s.%s = %s", actor, prop, v);
      Buffer value(64, v);
      bot->setProp(actor, prop, &value);
      return Executed;
    } else if (strcmp("get", c) == 0) {
      log(CLASS_MODULE, Info, "-> Get");
      const char *actor = strtok(NULL, " ");
      getProps(actor);
      return Executed;
    } else if (strcmp("int", c) == 0) {
      log(CLASS_MODULE, Info, "Interrupt");
      return ExecutedInterrupt;
    } else if (strcmp("mode", c) == 0) {
      const char *m = strtok(NULL, " ");
      if (m == NULL) {
        logUser("Mode: %s", getBot()->getModeName());
        return Executed;
      } else if (strcmp("run", m) == 0) {
        log(CLASS_MODULE, Info, "-> Run mode");
        runCmd();
        return ExecutedInterrupt;
      } else if (strcmp("conf", m) == 0) {
        log(CLASS_MODULE, Info, "-> Configure mode");
        confCmd();
        return ExecutedInterrupt;
      } else {
        return InvalidArgs;
      }
    } else if (strcmp("info", c) == 0) {
      infoCmd();
      return Executed;
    } else if (strcmp("test", c) == 0) {
      test();
      return Executed;
    } else if (strcmp("update", c) == 0) {
      const char *targetVersion = strtok(NULL, " ");
      if (targetVersion == NULL) {
        logRawUser("Arguments needed:\n  update <targetVersion>");
        return InvalidArgs;
      }
      update(targetVersion, STRINGIFY(PROJ_VERSION));
      return Executed;
    } else if (strcmp("clear", c) == 0) {
      clearDevice();
      return Executed;
    } else if (strcmp("logl", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        char ll = getLogLevel();
        log(CLASS_MODULE, Info, "Log level: %d", ll);
      } else {
        int ll = atoi(c);
        setLogLevel(ll);
        log(CLASS_MODULE, Info, "Log level: %d", ll);
      }
      logUser("Visible from now on:");
      log(CLASS_MODULE, Error, "- Error");
      log(CLASS_MODULE, Warn, "- Warn");
      log(CLASS_MODULE, Info, "- Info");
      log(CLASS_MODULE, Debug, "- Debug");
      return Executed;
    } else if (strcmp("wifissid", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  wifissid <ssid>");
        return InvalidArgs;
      }
      settings->setSsid(c);
      log(CLASS_MODULE, Info, "Wifi ssid: %s", settings->getSsid());
      return Executed;
    } else if (strcmp("wifipass", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  wifipass <pass>");
        return InvalidArgs;
      }
      settings->setPass(c);
      log(CLASS_MODULE, Info, "Wifi pass: %s", settings->getPass());
      return Executed;
    } else if (strcmp("wifissidb", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  wifissidb <ssid>");
        return InvalidArgs;
      }
      settings->setSsidBackup(c);
      log(CLASS_MODULE, Info, "Wifi ssidb: %s", settings->getSsidBackup());
      return Executed;
    } else if (strcmp("wifipassb", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  wifipassb <pass>");
        return InvalidArgs;
      }
      settings->setPassBackup(c);
      log(CLASS_MODULE, Info, "Wifi passb: %s", settings->getPassBackup());
      return Executed;
    } else if (strcmp("wifi", c) == 0) {
      initWifi();
      return Executed;
    } else if (strcmp("wifistop", c) == 0) {
      stopWifi();
      return Executed;
    } else if (strcmp("actall", c) == 0) {
      actall();
      return Executed;
    } else if (strcmp("touchall", c) == 0) {
      touchall();
      return Executed;
    } else if (strcmp("actone", c) == 0) {
      c = strtok(NULL, " ");
      if (c == NULL) {
        logRawUser("Argument needed:\n  actone <actorname>");
        return InvalidArgs;
      }
      actone(c);
      return Executed;
    } else if (strcmp("store", c) == 0) {
      propSync->fsStoreActorsProps(); // store credentials
      log(CLASS_MODULE, Info, "Properties stored locally");
      return Executed;
    } else if (strcmp("save", c) == 0) {
      const char *fname = strtok(NULL, " ");
      const char *content = strtok(NULL, "?????");
      bool suc = fileWrite(fname, content);
      log(CLASS_MODULE, Info, "File '%s' saved: %d", fname, (int)suc);
      return Executed;
#ifdef INSECURE
    } else if (strcmp("cat", c) == 0) { // could be potentially used to display credentials
      const char *f = strtok(NULL, " ");
      if (f == NULL) {
        logRawUser("Argument needed:\n  cat <file>");
        return InvalidArgs;
      }
      Buffer buf(128);
      fileRead(f, &buf);
      logUser("### File: %s", f);
      logRawUser(buf.getBuffer());
      logUser("###");
      return Executed;
#endif // INSECURE
    } else if (strcmp("load", c) == 0) {
      propSync->fsLoadActorsProps(); // load mainly credentials already set
      log(CLASS_MODULE, Info, "Properties loaded from local copy");
      return Executed;
    } else if (strcmp("help", c) == 0 || strcmp("?", c) == 0) {
      logRawUser(HELP_COMMAND_CLI);
      return commandArchitecture("?");
    } else {
      return commandArchitecture(c);
    }
  }

public:
  void cycleBot(bool mode, bool set, bool cycle) {
    TimingInterrupt interruptType = TimingInterruptNone;
    if (cycle) {
      interruptType = TimingInterruptCycle;
    }
    // execute a cycle on the bot
    bot->cycle(mode, set, interruptType);
  }

  // All getters should be removed, and initialization of these instances below should
  // be done in Module itself. This should help decrease the size of
public:
  SerBot *getBot() {
    return bot;
  }

public:
  Clock *getClock() {
    return clock;
  }

public:
  ClockSync *getClockSync() {
    return clockSync;
  }

public:
  PropSync *getPropSync() {
    return propSync;
  }

public:
  Settings *getSettings() {
    return settings;
  }

  /**
   * Make all actors act
   */
public:
  void actall() {
    for (int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      log(CLASS_MODULE, Info, "One off: %s", a->getName());
      a->oneOff();
    }
  }

  /**
   * Touch all actors (to force them to be syncrhonized)
   */
public:
  void touchall() {
    for (int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      Metadata *m = a->getMetadata();
      log(CLASS_MODULE, Info, "Touch: %s", a->getName());
      m->changed();
    }
  }

  /**
   * Make a given by-name-actor act
   */
public:
  void actone(const char *actorName) {
    for (int i = 0; i < getBot()->getActors()->size(); i++) {
      Actor *a = getBot()->getActors()->get(i);
      if (strcmp(a->getName(), actorName) == 0) {
        log(CLASS_MODULE, Info, "One off: %s", a->getName());
        a->oneOff();
      }
    }
  }

public:
  void runCmd() {
    bot->setMode(RunMode);
  }

public:
  void confCmd() {
    bot->setMode(ConfigureMode);
  }

public:
  void infoCmd() {
    info();
  }

public:
  void getProps(const char *actorN) {
    Buffer contentAuxBuffer(64);
    Array<Actor *> *actors = bot->getActors();
    for (int i = 0; i < actors->size(); i++) {
      Actor *actor = actors->get(i);
      log(CLASS_MODULE, Info, "# '%s'", actor->getName());
      if (actorN == NULL || strcmp(actor->getName(), actorN) == 0) {
        for (int p = 0; p < actor->getNroProps(); p++) {
          const char *propName = actor->getPropName(p);
          if (propName[0] == SENSITIVE_PROP_PREFIX[0]) { // sensitive
            contentAuxBuffer.fill("(sensitive)");
          } else {
            actor->getPropValue(p, &contentAuxBuffer);
          }
          log(CLASS_MODULE, Info, " '%s'='%s'", propName, contentAuxBuffer.getBuffer());
        }
      }
    }
  }

public:
  void configureMode() {
    time_t cycleBegin = now();
    configureModeArchitecture();
    sleepInterruptable(cycleBegin, PERIOD_CONFIGURE_MSEC / 1000);
  }

public:
  void runMode() {
    time_t cycleBegin = now();
    runModeArchitecture();
    cycleBot(false, false, true);
    if (oneRunModeSafe()) {
      // before finishing store in the server the last status of all actors
      // this includes the timing of the clock, that has progressed
      // and will allow the next run to start from where we left off
      log(CLASS_MODULE, Info, "Pushing actors to server (onerun)...");
      // push properties to the server (with new props and new clock blocked timing)
      getPropSync()->pushActors(true);
      deepSleepNotInterruptable(cycleBegin, getSettings()->periodMsec() / 1000);
    } else {
      sleepInterruptable(cycleBegin, getSettings()->periodMsec() / 1000);
    }
  }

private:
  bool oneRunModeSafe() {
    return (oneRunMode == NULL ? false : oneRunMode());
  }

public:
  Array<Actor *> *getActors() {
    return actors;
  }

public:
  void loop() {
    switch (getBot()->getMode()) {
      case (RunMode):
        log(CLASS_MODULE, Info, "\n\nBEGIN LOOP (ver: %s)", STRINGIFY(PROJ_VERSION));
        runMode();
        log(CLASS_MODULE, Info, "END LOOP\n\n");
        break;
      case (ConfigureMode):
        configureMode();
        break;
      default:
        break;
    }
  }
};

#endif // MODULE_INC
