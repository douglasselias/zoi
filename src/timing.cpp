
struct Time
{
  s64 begin;
  s64 frequency;
};

Time init_time()
{
  Time time = {};
  time.begin     = get_os_timer();
  time.frequency = get_os_timer_frequency();
  return time;
}

f32 calculate_dt(Time *time)
{
  s64 end_time = get_os_timer();
  f32 dt = (f32)((f64)(end_time - time->begin) / (f64)time->frequency);
  time->begin = end_time;

  return dt;
}

struct Frame
{
  s32 counter;
  f32 rate_timer;
  f64 time;
  f32 rate;
};

void frame_tick(Frame *frame, f32 dt)
{
  frame->time += dt;
  frame->rate_timer += dt;
  frame->counter++;

  if(frame->rate_timer >= 1)
  {
    frame->rate = (f32)frame->counter / frame->rate_timer;
    frame->counter = 0;
    frame->rate_timer = 0;
  }
}