/* person_test.c - flickcurl person XML parsing tests */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define FLICKCURL_INTERNAL 1
#include "flickcurl.h"
#include "flickcurl_internal.h"

static const char* const fixture_paths[] = {
  "../tests/fixtures/person-minimal.xml",
  "tests/fixtures/person-minimal.xml",
  NULL
};

static char*
read_fixture(void)
{
  const char* const* path;
  FILE* fh;
  char* xml = NULL;
  size_t len = 0;
  size_t size = 0;
  char buf[512];

  for(path = fixture_paths; *path; path++) {
    fh = fopen(*path, "r");
    if(!fh)
      continue;

    while(fgets(buf, sizeof(buf), fh)) {
      size_t n = strlen(buf);
      char* tmp;

      tmp = realloc(xml, len + n + 1);
      if(!tmp) {
        free(xml);
        fclose(fh);
        return NULL;
      }
      xml = tmp;
      memcpy(xml + len, buf, n);
      len += n;
      xml[len] = '\0';
    }
    fclose(fh);
    return xml;
  }

  return NULL;
}

int
main(void)
{
  flickcurl* fc;
  char* person_xml;
  xmlDocPtr doc;
  xmlXPathContextPtr xpathCtx;
  flickcurl_person* person;
  int failures = 0;

  person_xml = read_fixture();
  if(!person_xml)
    return 1;

  flickcurl_init();
  fc = flickcurl_new();
  if(!fc) {
    failures++;
    goto tidy_xml;
  }

  doc = xmlReadMemory(person_xml, (int)strlen(person_xml), "fixture:",
                      NULL, XML_PARSE_NONET);
  if(!doc) {
    failures++;
    goto tidy;
  }

  xpathCtx = xmlXPathNewContext(doc);
  if(!xpathCtx) {
    failures++;
    goto tidy_doc;
  }

  person = flickcurl_build_person(fc, xpathCtx, (const xmlChar*)"/rsp/person");
  if(!person) {
    failures++;
    goto tidy_xpath;
  }

  if(!person->nsid || strcmp(person->nsid, "987654321@N00"))
    failures++;

  if(person->fields[PERSON_FIELD_username].type != VALUE_TYPE_STRING ||
     !person->fields[PERSON_FIELD_username].string ||
     strcmp(person->fields[PERSON_FIELD_username].string, "fixtureuser"))
    failures++;

  if(person->fields[PERSON_FIELD_realname].type != VALUE_TYPE_STRING ||
     !person->fields[PERSON_FIELD_realname].string ||
     strcmp(person->fields[PERSON_FIELD_realname].string, "Example User"))
    failures++;

  flickcurl_free_person(person);

 tidy_xpath:
  xmlXPathFreeContext(xpathCtx);
 tidy_doc:
  xmlFreeDoc(doc);
 tidy:
  if(fc)
    flickcurl_free(fc);
  flickcurl_finish();
 tidy_xml:
  free(person_xml);

  return failures ? 1 : 0;
}
