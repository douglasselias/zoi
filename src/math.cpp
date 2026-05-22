f32 EPSILON = 0.000001f;

struct V3 { f32 x, y, z; };


f32 dot(V3 a, V3 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

V3 cross(V3 a, V3 b)
{
  return
  {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x,
  };
}

V3 operator-(V3 a, V3 b)
{
  return
  {
    a.x - b.x,
    a.y - b.y,
    a.z - b.z,
  };
}

V3 operator+(V3 a, V3 b)
{
  return
  {
    a.x + b.x,
    a.y + b.y,
    a.z + b.z,
  };
}


V3 operator*(V3 a, f32 scalar)
{
  return
  {
    a.x * scalar,
    a.y * scalar,
    a.z * scalar,
  };
}

V3 operator/=(V3 &a, f32 scalar)
{
  a.x /= scalar;
  a.y /= scalar;
  a.z /= scalar;
  return a;
}


f32 length(V3 v)
{
  f32 l = dot(v, v);
  return sqrtf(l);
}


V3 normalize(V3 v)
{
  f32 l = length(v);

  if(l > EPSILON)
  {
    v /= l;
  }

  return v;
}

union V4
{
  struct { f32 x, y, z, w; };
  struct { V3 rgb; f32 w; };
};


V4 operator*(V4 v, f32 scalar)
{
  return
  {
    v.x * scalar,
    v.y * scalar,
    v.z * scalar,
    v.w * scalar,
  };
}

V4 operator+(V4 a, V4 b)
{
  return
  {
    a.x + b.x,
    a.y + b.y,
    a.z + b.z,
    a.w + b.w,
  };
}

V4 operator/=(V4 &a, f32 scalar)
{
  a.x /= scalar;
  a.y /= scalar;
  a.z /= scalar;
  a.w /= scalar;
  return a;
}


f32 dot(V4 a, V4 b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

f32 length(V4 v)
{
  f32 l = dot(v, v);
  return sqrtf(l);
}

V4 normalize(V4 v)
{
  f32 l = length(v);

  if(l > EPSILON)
  {
    v /= l;
  }

  return v;
}

u32 u32_from_v4(V4 v)
{
  u32 r = (u32)(v.x * 255) << 24;
  u32 g = (u32)(v.y * 255) << 16;
  u32 b = (u32)(v.z * 255) <<  8;
  u32 a = (u32)(v.w * 255) <<  0;
  return r | g | b | a;
}

u32 u32_from_v3(V3 v)
{
  u32 r = (u32)(v.x * 255) << 24;
  u32 g = (u32)(v.y * 255) << 16;
  u32 b = (u32)(v.z * 255) <<  8;
  return r | g | b;
}
