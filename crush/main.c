#include <stdio.h>

// http://dev.w3.org/csswg/css-syntax/#tokenizing-and-parsing-css


struct tokeniser;
typedef void(*state)(struct tokeniser*);

struct tokeniser {
    FILE* input;
    unsigned current;
    state state;
};


void state_data(struct tokeniser* t)
{
    switch(t->current)
    {
    }
}

void tokeniser_init(struct tokeniser* t, FILE* input)
{
    t->input = input;
    t->current = 0;
    t->state = state_data;
}

int main(int argc, const char * argv[])
{
    struct tokeniser t;
    tokeniser_init(&t, stdin);

    for (;;)
    {
        t.state(&t);
    }

    return 0;
}

