// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef HAD_CONFIG_H
#define HAD_CONFIG_H
#ifndef _HAD_ZIPCONF_H
#include "lib/zipconf.h"
#endif
/* BEGIN DEFINES */
/* #undef HAVE___PROGNAME */
/* #undef HAVE__CLOSE */
/* #undef HAVE__DUP */
/* #undef HAVE__FDOPEN */
/* #undef HAVE__FILENO */
/* #undef HAVE__SETMODE */
/* #undef HAVE__SNPRINTF */
/* #undef HAVE__STRDUP */
/* #undef HAVE__STRICMP */
/* #undef HAVE__STRTOI64 */
/* #undef HAVE__STRTOUI64 */
/* #undef HAVE__UMASK */
/* #undef HAVE__UNLINK */
/* #undef HAVE_ARC4RANDOM */
/* #undef HAVE_CLONEFILE */
#undef HAVE_COMMONCRYPTO
#undef HAVE_CRYPTO
/* #undef HAVE_FICLONERANGE */
#define HAVE_FILENO
#define HAVE_FSEEKO
#define HAVE_FTELLO
/* #undef HAVE_GETPROGNAME */
/* #undef HAVE_GNUTLS */
#undef HAVE_LIBBZ2
#undef HAVE_LIBLZMA
#define HAVE_LOCALTIME_R
/* #undef HAVE_MBEDTLS */
/* #undef HAVE_MKSTEMP */
/* #undef HAVE_NULLABLE */
/* #undef HAVE_OPENSSL */
/* #undef HAVE_SETMODE */
#define HAVE_SNPRINTF
#define HAVE_STRCASECMP
#define HAVE_STRDUP
/* #undef HAVE_STRICMP */
#define HAVE_STRTOLL
#define HAVE_STRTOULL
/* #undef HAVE_STRUCT_TM_TM_ZONE */
#define HAVE_STDBOOL_H
#define HAVE_STRINGS_H
#define HAVE_UNISTD_H
/* #undef HAVE_WINDOWS_CRYPTO */
#define SIZEOF_OFF_T 8
#define SIZEOF_SIZE_T 8
/* #undef HAVE_DIRENT_H */
#define HAVE_FTS_H
/* #undef HAVE_NDIR_H */
/* #undef HAVE_SYS_DIR_H */
/* #undef HAVE_SYS_NDIR_H */
/* #undef WORDS_BIGENDIAN */
#define HAVE_SHARED
/* END DEFINES */
#define PACKAGE "libzip"
#define VERSION "1.7.3"

#endif /* HAD_CONFIG_H */
