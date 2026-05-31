f32 EPSILON = 0.000001f;

f32 TURN_HALF = 0.5f;
f32 TAU       = 6.28318530717958647692f;

#define GENERIC template<typename T>

GENERIC T radian_from_turn(T turn) { return turn * (T)TAU; }

f32 blend(f32 a, f32 b, f32 t)
{
  return (1 - t) * a + t * b;
}

f32 clamp(f32 value, f32 min_value, f32 max_value)
{
  value = min(value, max_value);
  value = max(value, min_value);
  return value;
}

struct V2 { f32 x, y; };

V2 operator+(V2 a, V2 b)
{
  return
  {
    a.x + b.x,
    a.y + b.y,
  };
}

V2 operator*(V2 a, V2 b)
{
  return
  {
    a.x * b.x,
    a.y * b.y,
  };
}

V2 operator*(V2 a, f32 scalar)
{
  return
  {
    a.x * scalar,
    a.y * scalar,
  };
}

V2 operator/(V2 a, f32 scalar)
{
  return
  {
    a.x / scalar,
    a.y / scalar,
  };
}

struct V3 { f32 x, y, z; };
V3 UP_BASE_AXIS = {0, 0, 1};

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

V3 operator/(V3 a, f32 scalar)
{
  return
  {
    a.x / scalar,
    a.y / scalar,
    a.z / scalar,
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

  f32& operator[](u8 i) { return (&x)[i]; }
};

V4 blend(V4 a, V4 b, f32 t)
{
  V4 result = {};
  result.x = blend(a.x, b.x, t);
  result.y = blend(a.y, b.y, t);
  result.z = blend(a.z, b.z, t);
  result.w = blend(a.w, b.w, t);
  return result;
}


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

V4 v4_from_u32(u32 color)
{
  f32 r = (f32)((color & 0xff000000) >> 24);
  f32 g = (f32)((color & 0x00ff0000) >> 16);
  f32 b = (f32)((color & 0x0000ff00) >>  8);
  f32 a = (f32)((color & 0x000000ff) >>  0);
  return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
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

V4 V4_from(V3 v, f32 w = 0)
{
  return {v.x, v.y, v.z, w};
}

struct Camera { V3 position, target; };
struct CameraAxes { V3 forward, left, up; };

CameraAxes get_camera_axes(V3 position, V3 target, V3 up_base_axis = UP_BASE_AXIS)
{
  CameraAxes axes = {};

  // Camera's local +X axis (points away from target, opposite to view direction)
  axes.forward = normalize(position - target);
  axes.left    = cross(up_base_axis, axes.forward);
  axes.up      = cross(axes.forward, axes.left);

  // ASSERT(floats_are_equal(length(axes.forward), 1.0f), STR("Forward is not normalized"));

  return axes;
}

V3 rotate_by_axis_angle(V3 x, V3 axis, f32 angle)
{
  f32 radian = radian_from_turn(angle);

  // Using Euler-Rodrigues Formula
  // Based on https://github.com/raysan5/raylib/blob/63b988ade906adc7264a8bec8fccec5f47befb55/src/raymath.h#L888
  radian /= 2;

  V3 w   = normalize(axis) * sinf(radian);
  V3 wx  = cross(w, x);
  V3 wv  = wx * (cosf(radian) * 2.0f);
  V3 wwv = cross(w, wx) * 2.0f;

  return x + wv + wwv;
}

void rotate_camera_by_axis(Camera *camera, V3 axis, f32 angle)
{
  V3 direction = camera->target - camera->position;
  V3 target_position = rotate_by_axis_angle(direction, axis, angle);
  camera->target = camera->position + target_position;
}

struct CameraActions
{
  bool is_pressing_left;
  bool is_pressing_right;
  bool is_pressing_forward;
  bool is_pressing_backward;
  bool is_pressing_up;
  bool is_pressing_down;

  bool is_looking_left;
  bool is_looking_right;

  bool is_looking_up;
  bool is_looking_down;
};

void update_camera(CameraActions actions, Camera *camera, f32 dt)
{
  CameraAxes axes = get_camera_axes(camera->position, camera->target);
  V3 movement = {};

  f32 movement_speed = 8.75f * dt;

  if(actions.is_pressing_forward)
  {
    movement = axes.forward * -movement_speed;
  }

  if(actions.is_pressing_backward)
  {
    movement = axes.forward * movement_speed;
  }

  if(actions.is_pressing_left)
  {
    movement = axes.left * -movement_speed;
  }
  
  if(actions.is_pressing_right)
  {
    movement = axes.left * movement_speed;
  }

  if(actions.is_pressing_up)
  {
    movement = UP_BASE_AXIS * movement_speed;
  }
  
  if(actions.is_pressing_down)
  {
    movement = UP_BASE_AXIS * -movement_speed;
  }

  camera->position = camera->position + movement;
  camera->target   = camera->target   + movement;

  f32 rotation_speed  = 0.3f;
  f32 rotation_amount = rotation_speed * dt;

  if(actions.is_looking_left)
  {
    rotate_camera_by_axis(camera, UP_BASE_AXIS, rotation_amount);
  }

  if(actions.is_looking_right)
  {
    rotate_camera_by_axis(camera, UP_BASE_AXIS, -rotation_amount);
  }

  if(actions.is_looking_up)
  {
    rotate_camera_by_axis(camera, axes.left, rotation_amount);
  }

  if(actions.is_looking_down)
  {
    rotate_camera_by_axis(camera, axes.left, -rotation_amount);
  }
}

union Matrix
{
  f32 m[4][4];
  V4 rows[4];
  struct
  {
    f32
    m00, m01, m02, m03,
    m10, m11, m12, m13,
    m20, m21, m22, m23,
    m30, m31, m32, m33;
  };

  V4& operator[](u8 i) { return rows[i]; }

  f32& operator()(u8 row, u8 column) { return m[row][column]; }

  V4 col(u8 i)
  {
    return
    {
      m[0][i],
      m[1][i],
      m[2][i],
      m[3][i],
    };
  }

  void set_col(u8 i, V4 value)
  {
    m[0][i] = value.x;
    m[1][i] = value.y;
    m[2][i] = value.z;
    m[3][i] = value.w;
  }
};

// Matrix operator -(Matrix m)
// {
//   Matrix result = {};

//   for(u8 row = 0; row < 4; row++)
//   {
//     result[row] = -m[row];
//   }

//   return result;
// }

Matrix operator *(Matrix a, Matrix b)
{
  Matrix result = {};
  
  for(u8 row = 0; row < 4; row++)
  {
    for(u8 col = 0; col < 4; col++)
    {
      result[row][col] = dot(a[row], b.col(col));
    }
  }
  
  return result;
}

V4 operator *(Matrix m, V4 v)
{
  V4 result = {};
  
  for(u8 row = 0; row < 4; row++)
  {
    result[row] = dot(m[row], v);
  }
  
  return result;
}

V4 operator *(V4 v, Matrix m)
{
  V4 result = {};
  
  for(u8 col = 0; col < 4; col++)
  {
    result[col] = dot(v, m.col(col));
  }
  
  return result;
}

// Matrix transpose(Matrix matrix)
// {
//   Matrix result = {};

//   for(u8 i = 0; i < 4; i++)
//   {
//     for(u8 j = 0; j < 4; j++)
//     {
//       result[i][j] = matrix[j][i];
//     }
//   }

//   return result;
// }

Matrix ID_MATRIX =
{
  1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 1, 0,
  0, 0, 0, 1,
};

Matrix create_translation_matrix(f32 x, f32 y, f32 z)
{
  return
  {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    x, y, z, 1,
  };
}

Matrix create_translation_matrix(V3 v)
{
  return create_translation_matrix(v.x, v.y, v.z);
}

Matrix create_scale_matrix(f32 x, f32 y, f32 z)
{
  return
  {
    x, 0, 0, 0,
    0, y, 0, 0,
    0, 0, z, 0,
    0, 0, 0, 1,
  };
}

Matrix create_scale_matrix(V3 v = {1,1,1})
{
  return create_scale_matrix(v.x, v.y, v.z);
}

Matrix create_scale_matrix(f32 s)
{
  return create_scale_matrix(s, s, s);
}

Matrix create_x_axis_rotation_matrix(f32 angle)
{
  f32 radian = radian_from_turn(angle);
  f32 sine   = sinf(radian);
  f32 cosine = cosf(radian);

  return
  {
    1, 0,      0,      0,
    0, cosine, sine,   0,
    0, -sine,  cosine, 0,
    0, 0,      0,      1,
  };
}

Matrix create_y_axis_rotation_matrix(f32 angle)
{
  f32 radian = radian_from_turn(angle);
  f32 sine   = sinf(radian);
  f32 cosine = cosf(radian);

  return
  {
    cosine, 0, -sine,   0,
    0,      1,  0,      0,
    sine,   0,  cosine, 0,
    0,      0,  0,      1,
  };
}

Matrix create_z_axis_rotation_matrix(f32 angle)
{
  f32 radian = radian_from_turn(angle);
  f32 sine   = sinf(radian);
  f32 cosine = cosf(radian);

  return
  {
    cosine, sine,   0, 0,
    -sine,  cosine, 0, 0,
    0,      0,      1, 0,
    0,      0,      0, 1,
  };
}

Matrix create_xyz_axis_rotation_matrix(f32 angle_x, f32 angle_y, f32 angle_z)
{
  Matrix x = create_x_axis_rotation_matrix(angle_x);
  Matrix y = create_y_axis_rotation_matrix(angle_y);
  Matrix z = create_z_axis_rotation_matrix(angle_z);

  return x * y * z;
}

Matrix create_xyz_axis_rotation_matrix(V3 v)
{
  return create_xyz_axis_rotation_matrix(v.x, v.y, v.z);
}


// struct CameraAxes { V3 forward, left, up; };

// CameraAxes get_camera_axes(V3 position, V3 target, V3 up_base_axis = UP_BASE_AXIS)
// {
//   CameraAxes axes = {};

//   // Camera's local +X axis (points away from target, opposite to view direction)
//   axes.forward = normalize(position - target);
//   axes.left    = cross(up_base_axis, axes.forward);
//   axes.up      = cross(axes.forward, axes.left);

//   // ASSERT(floats_are_equal(length(axes.forward), 1.0f), STR("Forward is not normalized"));

//   return axes;
// }

Matrix create_view_matrix(V3 position, V3 target, V3 up_base_axis = UP_BASE_AXIS)
{
  CameraAxes axes = get_camera_axes(position, target, up_base_axis);

  V3 movement =
  {
    -dot(axes.left,    position),
    -dot(axes.up,      position),
    -dot(axes.forward, position),
  };

  return
  {
    axes.left.x, axes.up.x,  axes.forward.x, 0,
    axes.left.y, axes.up.y,  axes.forward.y, 0,
    axes.left.z, axes.up.z,  axes.forward.z, 0,
    movement.x,  movement.y, movement.z,     1,
  };
}

Matrix create_perspective_projection_matrix(f32 aspect_ratio, f32 fov_y, f32 z_near)
{
  f32 radian = radian_from_turn(fov_y);
  f32 focal_length = 1 / tanf(radian / 2);
  f32 x_scale = focal_length / aspect_ratio;
  f32 y_scale = focal_length;

  f32 perspective_term = -1;
  f32 focal_range      = 0;
  f32 focal_offset     = z_near;

  return
  {
    x_scale, 0,       0,            0,
    0,       y_scale, 0,            0,
    0,       0,       focal_range,  perspective_term,
    0,       0,       focal_offset, 0,
  };
}

V3 ndc_to_screen(V3 v, u32 width, u32 height)
{
  V3 result = {};
  result.x = ((v.x + 1) / 2) * width;
  result.y = ((1 - v.y) / 2) * height;
  result.z = v.z; // TODO:
  return result;
}