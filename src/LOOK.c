/* Look-controller stubs and eyeball-target trampolines */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u_int;

float s_fEyeOffset;

extern void *LOOK_eye_cont(void *a0, void *a1, void *a2, int a3);

/* No-op init hook */
void LOOK_target_init(void)
{
}

/* No-op per-frame hook */
void LOOK_target_doit(void)
{
}

/* Left-eye entry point: offsets by +0.04 and dispatches into the shared eye controller */
void *LOOK_eyeL_cont(void *a0, void *a1, void *a2)
{
    s_fEyeOffset = 0.03999999911f;
    return LOOK_eye_cont(a0, a1, a2, 0);
}

/* Right-eye entry point: offsets by -0.04 and dispatches into the shared eye controller */
void *LOOK_eyeR_cont(void *a0, void *a1, void *a2)
{
    s_fEyeOffset = -0.03999999911f;
    return LOOK_eye_cont(a0, a1, a2, 1);
}
