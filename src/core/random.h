/**
 * file:    random.h
 * created: 2018-05-13
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

struct Random {
	u32 state;
	u32 seed;
};

Random create_random(u32 seed);
u32 next_u32(Random *r);
i32 next_i32(Random *r);
f32 next_f32(Random *r);
