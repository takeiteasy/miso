//
//  sprite.c
//  worms
//
//  Created by George Watson on 29/05/2021.
//  Copyright © 2021 George Watson. All rights reserved.
//

#include "sprite.h"

static int chroma_key(int x, int y, int c) {
  return c == BLACK ? 0 : c;
}

static const char *jsmn_type_str(jsmntype_t t) {
  switch (t) {
    case JSMN_UNDEFINED:
    default:
      return "UNDEFINED";
    case JSMN_OBJECT:
      return "OBJECT";
    case JSMN_ARRAY:
      return "ARRAY";
    case JSMN_STRING:
      return "STRING";
    case JSMN_PRIMITIVE:
      return "PRIMITIVE";
  }
}

#define JSON_CHECK(s) (t[i].type == JSMN_STRING && (int)strlen((s)) == t[i].end - t[i].start && !strncmp(json + t[i].start, (s), t[i].end - t[i].start))
#define JSON_EXPECT(x) \
  if (t[++i].type != (x)) { \
    printf("JSON ERROR: Expected %s at #%d, got %s\n", jsmn_type_str((x)), i, jsmn_type_str(t[i].type)); \
    goto ERROR; \
  }
#define JSON_TEST(x) \
  if (t[++i].type != (x)) { \
    i--; \
    break; \
  }
#define JSON_MAP(x) \
  do { \
    const char *end = json + t[i].end; \
    json_enable_debug(3, stdout); \
    int r = json_read_object(json + t[i].start, (x), &end); \
    if (r) { \
      printf("JSON ERROR: %s\n", json_error_string(r)); \
      goto ERROR; \
    } \
  } while(false);

static char* dir(char *p) {
  static const char dot[] = ".";
  char *ls = p != NULL ? strrchr (p, '/') : NULL;
  
  if (ls == p)
    ++ls;
  else if (ls != NULL && ls[1] == '\0')
    ls = memchr(p, ls - p, '/');
  
  if (ls != NULL)
    ls[0] = '\0';
  else
    p = (char*)dot;
  
  return p;
}

static const char* ext(const char *p) {
  const char *dot = strrchr(p, '.');
  return !dot || dot == p ? "" : dot + 1;
}

bool sprite(struct sprite_t *s, const char *p) {
  s->animation.frames = NULL;
  s->animation.n_frames = 0;
  s->atlas.frames = NULL;
  s->atlas.n_frames = 0;
  s->keyframe = 0;
  s->timer = 0.f;
  s->animated = false;
  
  char full[PATH_MAX];
  const char *pext = ext(p);
  if (!strcmp(pext, "bmp")) {
    sprintf(full, "%s", p);
    goto SKIP;
  }
  if (strcmp(pext, "json")) {
    printf("ERROR: Invalid file type\n");
    return false;
  }
  
  FILE *f = fopen(p, "rb");
  if (!f)
    return false;
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *json = malloc(fsize + 1);
  fread(json, 1, fsize, f);
  fclose(f);
  json[fsize] = '\0';
  
  jsmn_parser jp;
  jsmntok_t t[512];
  jsmn_init(&jp);
  int r = jsmn_parse(&jp, json, fsize, t, 512);
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    printf("JSON ERROR: Object expected at index 0\n");
ERROR:
    free(json);
    sprite_destroy(s);
    return false;
  }
  
  char img[PATH_MAX];
  for (int i = 1; i < r; ++i) {
    if (JSON_CHECK("frames")) {
      JSON_EXPECT(JSMN_ARRAY);
      s->atlas.n_frames = 0;
      s->atlas.frames = NULL;
      for (;;) {
        JSON_TEST(JSMN_OBJECT);
        s->atlas.frames = s->atlas.frames ? realloc(s->atlas.frames, (s->atlas.n_frames + 1) * sizeof(struct frame_t)) : malloc(sizeof(struct frame_t));
        int cf = s->atlas.n_frames++;
        
        struct json_attr_t attr_ignore[] = {
          { "", t_ignore },
          { NULL }
        };
        struct json_attr_t attr_inner[] = {
          { "x", t_integer, .addr.integer = &s->atlas.frames[cf].x },
          { "y", t_integer, .addr.integer = &s->atlas.frames[cf].y },
          { "w", t_integer, .addr.integer = &s->atlas.frames[cf].w },
          { "h", t_integer, .addr.integer = &s->atlas.frames[cf].h },
          { NULL }
        };
        struct json_attr_t attr[] = {
          { "filename", t_ignore },
          { "frame", t_object, .addr.attrs = attr_inner },
          { "rotated", t_ignore },
          { "trimmed", t_ignore },
          { "spriteSourceSize", t_object, .addr.attrs = attr_ignore },
          { "sourceSize", t_object, .addr.attrs = attr_ignore },
          { "duration", t_integer, .addr.integer = &s->atlas.frames[cf].duration },
          { NULL }
        };
        JSON_MAP(attr);
        i += 34;
      }
    } else if (JSON_CHECK("meta")) {
      JSON_TEST(JSMN_OBJECT);
      struct json_attr_t attr_inner[] = {
        { "w", t_integer, .addr.integer = &s->w },
        { "h", t_integer, .addr.integer = &s->h },
        { NULL }
      };
      struct frameTag {
        char name[64];
        int from, to;
      };
      struct frameTags {
        struct frameTag tags[128];
        int n_tags;
      } frameTags;
      struct json_attr_t attr_array[] = {
        { "name", t_string, STRUCTOBJECT(struct frameTag, name), .len = 64 },
        { "from", t_integer, STRUCTOBJECT(struct frameTag, from) },
        { "to", t_integer, STRUCTOBJECT(struct frameTag, to) },
        { "direction", t_ignore },
        { NULL }
      };
      struct json_attr_t attr[] = {
        { "app", t_ignore },
        { "version", t_ignore },
        { "image", t_string,  .addr.string = img,
                              .len = PATH_MAX },
        { "format", t_check, .dflt.check = "RGBA8888" },
        { "size", t_object, .addr.attrs = attr_inner },
        { "scale", t_ignore },
        { "frameTags", t_array, STRUCTARRAY(frameTags.tags,
                                            attr_array,
                                            &frameTags.n_tags) },
        { NULL }
      };
      JSON_MAP(attr);
      
      s->animation.frames = calloc(frameTags.n_tags, sizeof(struct keyframe_t));
      s->animation.n_frames = frameTags.n_tags;
      for (int j = 0; j < frameTags.n_tags; ++j) {
        size_t name_sz = strlen(frameTags.tags[j].name);
        s->animation.frames[j] = (struct keyframe_t){
          .name = calloc(name_sz + 1, sizeof(char)),
          .from = frameTags.tags[j].from,
          .to   = frameTags.tags[j].to
        };
        sprintf(s->animation.frames[j].name, "%s", frameTags.tags[j].name);
        s->animation.frames[j].name[name_sz] = '\0';
      }
      break;
    } else {
      printf("JSON ERROR: Invalid key at #%d, expected STRING (frames/meta), got %s\n", i, jsmn_type_str(t[i].type));
      goto ERROR;
    }
  }
  free(json);
  s->frame = s->animation.n_frames ? &s->animation.frames[0] : NULL;
  
  char path[PATH_MAX];
  sprintf(path, "%s", p);
  sprintf(full, "%s/%s", dir(path), img);
  
  bool ok;
SKIP:
  ok = bmp(&s->buffer, full);
  if (!ok) {
    sprite_destroy(s);
    return false;
  }
  passthru(&s->buffer, chroma_key);
  return true;
}

bool spirte_frame(struct sprite_t *s, const char *f) {
  if (!f)
    return s->frame = s->animation.n_frames ? &s->animation.frames[0] : NULL;
  for (int i = 0; i < s->animation.n_frames; ++i)
    if (!strcmp(f, s->animation.frames[i].name)) {
      s->frame = &s->animation.frames[i];
      s->keyframe = s->frame->from;
      s->timer = 0.f;
      return true;
    }
  return false;
}

void sprite_update(struct sprite_t *s, float dt) {
  if (!s->frame)
    return;
  
  s->timer += dt;
  float d = s->atlas.frames[s->keyframe].duration;
  if (s->timer > d) {
    if (++s->keyframe > s->frame->to)
      s->keyframe = s->frame->from;
    s->timer -= d;
  }
}

void sprite_draw(struct sprite_t *s, int x, int y, struct surface_t *b) {
  struct frame_t *frame = &s->atlas.frames[s->keyframe];
  if (frame)
    clip_paste(b, &s->buffer, x, y, frame->x, frame->y, frame->w, frame->h);
  else
    paste(b, &s->buffer, x, y);
}

void sprite_destroy(struct sprite_t *s) {
  if (s->animation.frames) {
    for (int i = 0; i < s->animation.n_frames; ++i)
      free(s->animation.frames[i].name);
    free(s->animation.frames);
  }
  if (s->atlas.frames)
    free(s->atlas.frames);
  surface_destroy(&s->buffer);
}
