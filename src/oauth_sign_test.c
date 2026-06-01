/* oauth_sign_test.c - OAuth signing behaviour tests */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define FLICKCURL_INTERNAL 1
#include "flickcurl.h"
#include "flickcurl_internal.h"

int
main(void)
{
  flickcurl* fc;
  const char* uri;
  int failures = 0;

  flickcurl_init();
  fc = flickcurl_new();
  if(!fc)
    return 1;

  flickcurl_set_oauth_client_key(fc, "client-key");
  flickcurl_set_oauth_client_secret(fc, "client-secret");
  flickcurl_set_oauth_token(fc, "access-token");
  flickcurl_set_oauth_token_secret(fc, "access-secret");

  flickcurl_init_params(fc, 0);
  flickcurl_end_params(fc);

  if(flickcurl_prepare_noauth(fc, "flickr.test.null"))
    failures++;

  uri = fc->uri;
  if(!uri || !strstr(uri, "oauth_signature="))
    failures++;

  if(fc)
    flickcurl_free(fc);
  flickcurl_finish();

  return failures ? 1 : 0;
}
