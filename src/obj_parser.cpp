union Indexes
{
  struct { u32 v, u, n; };
  u32 e[3];
};

struct Face
{
  Indexes indexes[4];
};

struct Obj
{
  V3 *vertices;
  u32 vertices_count;
  V2 *uvs;
  u32 uvs_count;
  V3 *normals;
  u32 normals_count;
  Face *faces;
  u32 faces_count;
};

Obj parse_obj(char *file_path)
{
  FILE *file = fopen(file_path, "r");

  fseek(file, 0, SEEK_END);
  u64 file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *content = (char*)malloc(file_size + 1);
  fread(content, 1, file_size, file);
  content[file_size] = '\0';
  fclose(file);

  Obj obj = {};

  u32 lines_count = 0;
  String content_string = {content, (u32)file_size};
  String *lines = split(content_string, '\n', &lines_count);

  for(u32 i = 0; i < lines_count; i++)
  {
    String line = lines[i];
    u32 unused = 0;
    String *line_contents = split(line, ' ', &unused);

    if(line_contents[0] == S("v"))
    {
      f32 vertices[3] = {};
      for(u32 j = 0; j < 3; j++)
      {
        vertices[j] = (f32)atof(line_contents[j + 1].text);
      }

      obj.vertices_count += 1;
      obj.vertices = (V3*)realloc(obj.vertices, obj.vertices_count * sizeof(V3));
      memcpy(obj.vertices + obj.vertices_count - 1, vertices, sizeof(V3));
    }
    else if(line_contents[0] == S("vt"))
    {
      f32 uvs[2] = {};
      for(u32 j = 0; j < 2; j++)
      {
        uvs[j] = (f32)atof(line_contents[j + 1].text);
      }

      obj.uvs_count += 1;
      obj.uvs = (V2*)realloc(obj.uvs, obj.uvs_count * sizeof(V2));
      memcpy(obj.uvs + obj.uvs_count - 1, uvs, sizeof(V2));
    }
    else if(line_contents[0] == S("vn"))
    {
      f32 normals[3] = {};
      for(u32 j = 0; j < 3; j++)
      {
        normals[j] = (f32)atof(line_contents[j + 1].text);
      }

      obj.normals_count += 1;
      obj.normals = (V3*)realloc(obj.normals, obj.normals_count * sizeof(V3));
      memcpy(obj.normals + obj.normals_count - 1, normals, sizeof(V3));
    }
    else if(line_contents[0] == S("f"))
    {      
      Face face = {};
      for(u32 j = 0; j < 4; j++)
      {
        u32 unused_count = 0;
        String *indexes_string = split(line_contents[j + 1], '/', &unused_count);
        for(u32 k = 0; k < 3; k++)
        {
          face.indexes[j].e[k] = (u32)atoi(indexes_string[k].text);
        }
      }

      obj.faces_count++;
      obj.faces = (Face*)realloc(obj.faces, obj.faces_count * sizeof(Face));
      memcpy(&obj.faces[obj.faces_count - 1], &face, sizeof(Face));
    }
  }

  free(content);
  return obj;
}