/**
 * file:    random.cpp
 * created: 2017-03-08
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

// TODO(jesper): better random!
u32 next_u32(Random *r)
{
	u32 x = r->state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

	r->state = x;
	return x - 1;
}

// TODO(jesper): better random!
i32 next_i32(Random *r)
{
	return (i32)next_u32(r);
}

// TODO(jesper): better random!
f32 next_f32(Random *r)
{
	return (f32)next_u32(r) / 0x7FFFFFFF;
}

Random create_random(u32 seed)
{
	Random rand;
	rand.state = seed;
	rand.seed  = seed;

	for (i32 i = 0; i < 30; i++) {
		next_u32(&rand);
	}

	return rand;
}
