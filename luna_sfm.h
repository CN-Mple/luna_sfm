/* luna_sfm.h */
#ifndef __LUNA_SFM_H
#define __LUNA_SFM_H

#include <stdint.h>
#include <stdbool.h>

#ifndef LUNA_ASSERT
#include <assert.h>
#define LUNA_ASSERT                     assert
#endif

#ifndef LUNA_SFM_DEPTH
#define LUNA_SFM_DEPTH          (16)
#endif

typedef enum {
        PASS = 0,
        FAIL = !PASS
} Result;

typedef Result (*sfm_func_t)(void *me);

struct state {
        struct state *top;
        sfm_func_t init;
        sfm_func_t enter;
        sfm_func_t exec;
        sfm_func_t exit;
};

struct StateCtx {
        struct state *active;
        struct state *next;
        int nest;
        bool leaving;
};

void sfm_init(struct StateCtx *me);
Result sfm_run(struct StateCtx *me);
void sfm_set(struct StateCtx *me, struct state *s);

#endif

#define LUNA_SFM_IMPLEMENTATION
#ifdef LUNA_SFM_IMPLEMENTATION

static void sfm_tran(struct StateCtx *me, struct state *s);

static void leaf_enter(struct StateCtx *me, struct state *s, struct state *top);
static void leaf_leave(struct StateCtx *me, struct state *top);

static bool leaf_of(const struct state *a, const struct state *b);
static struct state *leaf_get(struct state *a, struct state *b);
static struct state *lca_of(struct state *a, struct state *b);

void sfm_init(struct StateCtx *me)
{
	LUNA_ASSERT(me != 0);

        me->leaving = false;
        me->nest = 0;

        Result result;
        struct state *s;
        struct state *path[LUNA_SFM_DEPTH];
        uint32_t p = 0;

        s = me->active;
        while(s) {
                path[p++] = s;
                s = s->top;
        }
        while(p--) {
                s = path[p];
                if (s->init) {
                        result = s->init(s);
                }
        }
}


Result sfm_run(struct StateCtx *me)
{
	LUNA_ASSERT(me != 0);
	LUNA_ASSERT(me->active != 0);


        struct state *s = me->active;
        Result result = FAIL;
        while (s) {
                if (s->exec) {
                        result = s->exec(me);
                        if (result == PASS) {
                                break;
                        }
                }
                s = s->top;
        }

        if (me->next != NULL) {
                sfm_tran(me, me->next);
                me->next = NULL;
        }
        return result;
}

void sfm_set(struct StateCtx *me, struct state *s)
{
        LUNA_ASSERT(me != NULL);
        LUNA_ASSERT(s != NULL);
        me->next = s;
}

static void sfm_tran(struct StateCtx *me, struct state *s)
{
	LUNA_ASSERT(me != 0);
	LUNA_ASSERT(me->active != 0);
	LUNA_ASSERT(s != 0);

        me->leaving = false;

        struct state *top;
        if (leaf_of(me->active, s)) {
                top = s;
        } else if (leaf_of(s, me->active)) {
                top = me->active;
        } else {
                top = lca_of(me->active, s);
        }

        leaf_leave(me, top);
        leaf_enter(me, s, top);
}

static bool leaf_of(const struct state *a, const struct state *b)
{
        if (a == b || !a) {
		return false;
	}
        while (a) {
                if (a->top == b) {
			return true;
		}
                a = a->top;
        }
        return false;
}

static struct state *lca_of(struct state *a, struct state *b)
{
        struct state *s = a->top;
        while (s) {
		if (leaf_of(b, s)) {
			return s;
		}
                s = s->top;
        }
        return NULL;
}

static struct state *leaf_get(struct state *a, struct state *b)
{
        if (!a) {
		return NULL;
	}
        struct state *s = a;
        while (s) {
                if (s->top == b) {
			return s;
		}
                s = s->top;
        }
        return NULL;
}

static void leaf_enter(struct StateCtx *me, struct state *s, struct state *top)
{
        me->nest++;
        int nest = me->nest;
        me->active = top;

        for (struct state *_s = leaf_get(s, top); _s; _s = leaf_get(s, _s)) {
                me->active = _s;
                if (_s->enter) {
                        _s->enter(me);
                }
                if (nest < me->nest) {
                        me->nest--;
                        return;
                }
                if (_s == s) {
                        break;
                }
        }
        if (nest < me->nest) {
                me->nest--;
        }
}

static void leaf_leave(struct StateCtx *me, struct state *top)
{
        for (struct state *s = me->active; s && s != top; s = s->top) {
                if (s->exit) {
                        s->exit(me);
                }
        }
}

#endif
