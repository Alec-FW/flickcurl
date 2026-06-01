/* config_test.c - flickcurl config read/write tests */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "flickcurl.h"

static int
file_contains(const char* path, const char* needle)
{
  FILE* fh;
  char buf[512];

  fh = fopen(path, "r");
  if(!fh)
    return 0;
  while(fgets(buf, sizeof(buf), fh)) {
    if(strstr(buf, needle)) {
      fclose(fh);
      return 1;
    }
  }
  fclose(fh);
  return 0;
}

int
main(void)
{
  flickcurl* fc;
  char oauth_path[] = "/tmp/flickcurl-oauth-conf-XXXXXX";
  char legacy_path[] = "/tmp/flickcurl-legacy-conf-XXXXXX";
  int failures = 0;

  flickcurl_init();
  fc = flickcurl_new();
  if(!fc)
    return 1;

  if(!mkstemp(oauth_path)) {
    failures++;
    goto tidy;
  }
  unlink(oauth_path);

  flickcurl_set_oauth_client_key(fc, "client-key");
  flickcurl_set_oauth_client_secret(fc, "client-secret");
  flickcurl_set_oauth_token(fc, "access-token");
  flickcurl_set_oauth_token_secret(fc, "access-secret");

  if(flickcurl_config_write_ini(fc, oauth_path, "flickr"))
    failures++;

#ifndef _WIN32
  {
    struct stat st;
    if(stat(oauth_path, &st) != 0 || (st.st_mode & 0777) != 0600)
      failures++;
  }
#endif

  if(flickcurl_config_read_ini(fc, oauth_path, "flickr", fc,
                               flickcurl_config_var_handler))
    failures++;

  if(!flickcurl_get_oauth_token(fc) ||
     strcmp(flickcurl_get_oauth_token(fc), "access-token"))
    failures++;

  unlink(oauth_path);

  if(!mkstemp(legacy_path)) {
    failures++;
    goto tidy;
  }
  unlink(legacy_path);

  flickcurl_free(fc);
  fc = flickcurl_new();
  flickcurl_set_api_key(fc, "api-key");
  flickcurl_set_shared_secret(fc, "shared-secret");
  flickcurl_set_auth_token(fc, "legacy-token");

  if(flickcurl_config_write_ini(fc, legacy_path, "flickr"))
    failures++;

  if(!file_contains(legacy_path, "auth_token=legacy-token") ||
     !file_contains(legacy_path, "secret=shared-secret"))
    failures++;

  unlink(legacy_path);

 tidy:
  if(fc)
    flickcurl_free(fc);
  flickcurl_finish();

  return failures ? 1 : 0;
}
