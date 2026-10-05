/*
  Copyright (c) 2009-2019 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "../cJSON.h"
#ifdef TEST_CJSON_UTILS
#include "../cJSON_Utils.h"
#endif

int CJSON_CDECL main(void)
{
    cJSON *first = cJSON_Parse("{\"number\":1.25}");
    cJSON *second = cJSON_Parse("{\"number\":1.25}");
    int result = 1;

    if ((first != NULL) && (second != NULL))
    {
        result = cJSON_Compare(first, second, 1) ? 0 : 1;
#ifdef TEST_CJSON_UTILS
        if (cJSONUtils_GetPointer(first, "/number") == NULL)
        {
            result = 1;
        }
#endif
    }
    cJSON_Delete(first);
    cJSON_Delete(second);
    return result;
}
