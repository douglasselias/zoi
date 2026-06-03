struct String
{
  char *text;
  u32 size;
};

#define S(text) String{text, sizeof(text)-1}

bool operator==(String a, String b)
{
  if(a.size != b.size) return false;

  for(u32 i = 0; i < a.size; i++)
  {
    if(a.text[i] != b.text[i])
    {
      return false;
    }
  }
  return true;
}

String slice(String s, u32 begin, u32 end = 0)
{
  String result = {"", 0};
  if(end == 0) end = s.size;
  if(end > begin)
  {
    result.size = end - begin;
    result.text = s.text + begin;
  }
  return result;
}

struct SliceTestCase
{
  String input, output;
  u32 begin, end;
};

void test_slice()
{
  SliceTestCase test_cases[] =
  {
    {S("abc"), S("a"),   0, 1},
    {S("abc"), S("b"),   1, 2},
    {S("abc"), S("c"),   2, 3},
    {S("abc"), S("ab"),  0, 2},
    {S("abc"), S("bc"),  1, 3},
    {S("abc"), S("abc"), 0, 3},
    {S("abc"), S("abc"), 0, 0},
    {S("abc"), S(""),    3, 0},
  };

  u32 failed = 0;
  u32 array_count = sizeof(test_cases) / sizeof(test_cases[0]);
  for(u32 i = 0; i < array_count; i++)
  {
    SliceTestCase t = test_cases[i];
    String output = slice(t.input, t.begin, t.end);
    if(!(output == t.output))
    {
      failed++;
    }
  }
}

String* split(String s, char c, u32 *count)
{
  for(u32 i = 0; i < s.size; i++)
  {
    if(s.text[i] == c)
    {
      (*count)++;
    }
  }

  (*count)++;

  String *result = (String*)calloc(*count, sizeof(String));
  u32 begin = 0, index = 0;

  for(u32 i = 0; i < s.size; i++)
  {
    if(s.text[i] == c)
    {
      result[index++] = slice(s, begin, i);
      begin = i + 1;
    }
  }

  result[(*count) - 1] = slice(s, begin, s.size);

  return result;
}

struct SplitTestCase
{
  String input;
  String *output;
  u32 output_count;
  char c;
};

void test_split()
{
  String output0[] = {S("a"), S("b")};
  String output1[] = {S("a")};
  String output2[] = {S("a")};
  String output3[] = {S("a"), S("b")};
  SplitTestCase test_cases[] =
  {
    {S("a b"), output0, 2, ' '},
    {S("a "),  output1, 1, ' '},
    // {S("a  "), output2, 1, ' '},
    {S("a\nb"), output3, 2, '\n'},
  };

  u32 failed = 0;
  u32 array_count = sizeof(test_cases) / sizeof(test_cases[0]);
  for(u32 i = 0; i < array_count; i++)
  {
    SplitTestCase t = test_cases[i];
    u32 count = 0;
    String *output = split(t.input, t.c, &count);

    if(count != t.output_count)
    {
      failed++;
      continue;
    }

    for(u32 j = 0; j < count; j++)
    {
      if(!(output[j] == t.output[j]))
      {
        failed++;
      }
    }
  }
}