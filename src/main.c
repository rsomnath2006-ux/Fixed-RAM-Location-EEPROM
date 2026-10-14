#include "nvm demo.h"

int main(void)
{
    Demo_Init();

    for (;;)
    {
        Demo_Step();
    }
}
