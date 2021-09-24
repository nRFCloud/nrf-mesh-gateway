/* the configured options and settings */
#define HW_VERSION_MAJOR 0
#define HW_VERSION_MINOR 9
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 0
#define FW_VERSION_PATCH be522d5

#define REVSTR2(x)    #x
#define REVSTR(x)     REVSTR2(x)
#define HW_REV_STRING "v" REVSTR(HW_VERSION_MAJOR) "." REVSTR(HW_VERSION_MINOR)
#define FW_REV_STRING "v" REVSTR(FW_VERSION_MAJOR) "." REVSTR(FW_VERSION_MINOR) "." REVSTR(FW_VERSION_PATCH)
#define BUILT_STRING REVSTR(__DATE__) " " REVSTR(__TIME__)