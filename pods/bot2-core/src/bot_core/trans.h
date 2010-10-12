#ifndef __bot_trans_h__
#define __bot_trans_h__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup BotCoreTrans Trans
 * @ingroup BotCoreMathGeom
 * @brief Simple 3D rigid-body transformations
 * @include: bot_core/bot_core.h
 *
 * Data structure and functions for working with 3D rigid-body transformations.
 *
 * Linking: `pkg-config --libs bot2-core`
 *
 * @{
 */

/**
 * BotTrans:
 * @rot_quat: unit quaternion
 * @trans_vec: translation vector
 *
 * Represents a rigid-body transformation corresponding to a rotation followed
 * by a translation.
 */
typedef struct _BotTrans BotTrans;
struct _BotTrans
{
    double rot_quat[4];
    double trans_vec[3];
};

/**
 * bot_trans_set_identity:
 * @btrans: the #BotTrans to initialize
 *
 * Set a #BotTrans to the identity transform (rotation: <1, 0, 0, 0>  translation: <0, 0, 0>)
 */
void bot_trans_set_identity(BotTrans *btrans);

/**
 * bot_trans_copy:
 * @dest: output parameter
 * @src: source #BotTrans
 *
 * Makes a copy of a #BotTrans
 */
void bot_trans_copy(BotTrans *dest, const BotTrans *src);

/**
 * bot_trans_set_from_quat_trans:
 * @dest: output parameter
 * @rot_quat: input quaternion
 * @trans_vec: input translation vector
 *
 * Creates a BotTrans that represents first rotating by @rot_quat and then
 * translating by @trans_vec
 */
void bot_trans_set_from_quat_trans(BotTrans *dest, const double rot_quat[4],
        const double trans_vec[3]);

/**
 * bot_trans_interpolate:
 * @dest: output parameter
 * @trans_a: rigid body transformation A
 * @trans_b: rigid body transformation B
 * @weight_b: weighting parameter
 *
 * Interpolates two rigid body transformations.  The quaternion is interpolated 
 * via spherical interpolation on a 4-d sphere, and the translation vector is 
 * interpolated linearly.
 */
void bot_trans_interpolate(BotTrans *dest, const BotTrans * trans_a, 
        const BotTrans * trans_b, double weight_b);

/**
 * bot_trans_apply_trans:
 *
 * Modifies %dest so that it is equivalent to first applying the original
 * @dest, and then applying @src.  If we represent SRC and DEST as homogeneous
 * matrices, then this operation is equivalent to:  DEST = SRC * DEST
 */
void bot_trans_apply_trans(BotTrans *dest, const BotTrans * src);

/**
 * bot_trans_invert:
 *
 * Inverts the transformation.  Modifies @btrans in-place.
 */
void bot_trans_invert(BotTrans * btrans);

/**
 * bot_trans_rotate_vec:
 * @btrans: input rigid body transformation
 * @src: inpput vector
 * @dst: output vector
 *
 * Applies only the rotation portion of the transformation to a vector
 */
void bot_trans_rotate_vec(const BotTrans * btrans, 
        const double src[3], double dst[3]);

/**
 * bot_trans_apply_vec:
 * @btrans: input rigid body transformation
 * @src: inpput vector
 * @dst: output vector
 *
 * Applies the rigid body transformation to a vector.
 */
void bot_trans_apply_vec(const BotTrans * btrans, const double src[3],
        double dst[3]);

/**
 * bot_trans_get_rot_mat_3x3:
 *
 * Retrieves the 3x3 orthonormal rotation matrix corresponding to the rotation
 * portion of the rigid body transformation.
 */
void bot_trans_get_rot_mat_3x3(const BotTrans * btrans, double rot_mat[9]);

/**
 * bot_trans_get_mat_4x4:
 *
 * Retrieves a 4x4 homogeneous matrix representation of the rigid body
 * transformation
 */
void bot_trans_get_mat_4x4(const BotTrans *btrans, double mat[16]);

/**
 * bot_trans_get_mat_3x4:
 *
 * Retrieves a 3x4 matrix representation of the rigid body transformation,
 * suitable for use with bot_vector_affine_transform_3x4_3d().
 */
void bot_trans_get_mat_3x4(const BotTrans *btrans, double mat[12]);

/**
 * bot_trans_get_trans_vec:
 *
 * Retrieves the 3-dimensional translation vector corresponding to the
 * translation portion of the rigid body transformation.
 */
void bot_trans_get_trans_vec(const BotTrans * btrans, double trans_vec[3]);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
