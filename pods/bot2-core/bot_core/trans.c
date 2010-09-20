#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "small_linalg.h"
#include "rotations.h"
#include "trans.h"

void
bot_trans_set_identity(BotTrans *btrans)
{
    btrans->rot_quat[0] = 1;
    btrans->rot_quat[1] = 0;
    btrans->rot_quat[2] = 0;
    btrans->rot_quat[3] = 0;
    btrans->trans_vec[0] = 0;
    btrans->trans_vec[1] = 0;
    btrans->trans_vec[2] = 0;
}

void
bot_trans_copy(BotTrans * dest, const BotTrans *src)
{
    memcpy(dest, src, sizeof(BotTrans));
}

void
bot_trans_set_from_quat_trans(BotTrans *btrans, const double rot_quat[4],
        const double trans_vec[3])
{
    // TODO check that rot_quat is a valid quaternion
    memcpy(btrans->rot_quat, rot_quat, 4*sizeof(double));
    memcpy(btrans->trans_vec, trans_vec, 3*sizeof(double));
}

void
bot_trans_apply_trans(BotTrans *dest, const BotTrans * src)
{
    bot_quat_rotate(src->rot_quat, dest->trans_vec);
    double qtmp[4];
    bot_quat_mult(qtmp, src->rot_quat, dest->rot_quat);
    memcpy(dest->rot_quat, qtmp, 4*sizeof(double));
    dest->trans_vec[0] += src->trans_vec[0];
    dest->trans_vec[1] += src->trans_vec[1];
    dest->trans_vec[2] += src->trans_vec[2];
}

void
bot_trans_invert(BotTrans * btrans)
{
    btrans->trans_vec[0] = -btrans->trans_vec[0];
    btrans->trans_vec[1] = -btrans->trans_vec[1];
    btrans->trans_vec[2] = -btrans->trans_vec[2];
    bot_quat_rotate_rev(btrans->rot_quat, btrans->trans_vec);
    btrans->rot_quat[1] = -btrans->rot_quat[1];
    btrans->rot_quat[2] = -btrans->rot_quat[2];
    btrans->rot_quat[3] = -btrans->rot_quat[3];
}

void
bot_trans_interpolate(BotTrans *dest, const BotTrans * trans_a,
        const BotTrans * trans_b, double weight_b)
{
    bot_vector_interpolate_3d(trans_a->trans_vec, trans_b->trans_vec,weight_b,
        dest->trans_vec);
    bot_quat_interpolate(trans_a->rot_quat, trans_b->rot_quat, weight_b,
            dest->rot_quat);
}

void
bot_trans_rotate_vec(const BotTrans * btrans,
        const double src[3], double dst[3])
{
    bot_quat_rotate_to(btrans->rot_quat, src, dst);
//    bot_matrix_vector_multiply_3x3_3d(btrans->rot_mat, src, dst);
}

void
bot_trans_apply_vec(const BotTrans * btrans, const double src[3],
        double dst[3])
{
    bot_quat_rotate_to(btrans->rot_quat, src, dst);
    dst[0] += btrans->trans_vec[0];
    dst[1] += btrans->trans_vec[1];
    dst[2] += btrans->trans_vec[2];
}

void
bot_trans_get_rot_mat_3x3(const BotTrans * btrans, double rot_mat[9])
{
    bot_quat_to_matrix(btrans->rot_quat, rot_mat);
//    memcpy(rot_mat, btrans->rot_mat, 9*sizeof(double));
}

void
bot_trans_get_mat_4x4(const BotTrans *btrans, double mat[16])
{
    double tmp[9];
    bot_quat_to_matrix(btrans->rot_quat, tmp);

    // row 1
    memcpy(mat+0, tmp+0, 3*sizeof(double));
    mat[3] = btrans->trans_vec[0];

    // row 2
    memcpy(mat+4, tmp+3, 3*sizeof(double));
    mat[7] = btrans->trans_vec[1];

    // row 3
    memcpy(mat+8, tmp+6, 3*sizeof(double));
    mat[11] = btrans->trans_vec[2];

    // row 4
    mat[12] = 0;
    mat[13] = 0;
    mat[14] = 0;
    mat[15] = 1;
}

void
bot_trans_get_mat_3x4(const BotTrans *btrans, double mat[12])
{
    double tmp[9];
    bot_quat_to_matrix(btrans->rot_quat, tmp);

    // row 1
    memcpy(mat+0, tmp+0, 3*sizeof(double));
    mat[3] = btrans->trans_vec[0];

    // row 2
    memcpy(mat+4, tmp+3, 3*sizeof(double));
    mat[7] = btrans->trans_vec[1];

    // row 3
    memcpy(mat+8, tmp+6, 3*sizeof(double));
    mat[11] = btrans->trans_vec[2];
}

void
bot_trans_get_trans_vec(const BotTrans * btrans, double trans_vec[3])
{
    memcpy(trans_vec, btrans->trans_vec, 3*sizeof(double));
}
