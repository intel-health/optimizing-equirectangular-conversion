import numpy as np

from . import utils
import time


def e2p(e_img, fov_deg, u_deg, v_deg, out_hw, in_rot_deg=0, mode='bilinear'):
    '''
    e_img:   ndarray in shape of [H, W, *]
    fov_deg: scalar or (scalar, scalar) field of view in degree
    u_deg:   horizon viewing angle in range [-180, 180]
    v_deg:   vertical viewing angle in range [-90, 90]
    '''
    start_time = time.time()
    assert len(e_img.shape) == 3
    h, w = e_img.shape[:2]
    print("w = ", w)
    print("h = ", h)
    try:
        h_fov, v_fov = fov_deg[0] * np.pi / 180, fov_deg[1] * np.pi / 180
    except:
        h_fov, v_fov = fov_deg, fov_deg
    in_rot = in_rot_deg * np.pi / 180

    if mode == 'bilinear':
        order = 1
    elif mode == 'nearest':
        order = 0
    else:
        raise NotImplementedError('unknown mode')

    u = -u_deg * np.pi / 180
    v = v_deg * np.pi / 180
    setup_time = time.time()
    xyz = utils.xyzpers(h_fov, v_fov, u, v, out_hw, in_rot)
    xyzpers_time = time.time()
    print("xyz = ", xyz)
    # Generate the coordinates of points on the equirectangular sphere
    uv = utils.xyz2uv(xyz)
    #print("uv = ", uv)
    xyz2uv_time = time.time()
    # Convert those to x, y coordinates within the equirectangular image we loaded.
    coor_xy = utils.uv2coor(uv, h, w)
    uv2coor_time = time.time()
    pers_img = np.stack([
        utils.sample_equirec(e_img[..., i], coor_xy, order=order)
        for i in range(e_img.shape[2])
    ], axis=-1)

    end_time = time.time()
    print("setup_time time required = ", setup_time - start_time)
    print("xyzpers_time time required = ", xyzpers_time - setup_time)
    print("xyz2uv_time time required = ", xyz2uv_time - xyzpers_time)
    print("uv2coor_time ime required = ", uv2coor_time - xyz2uv_time)
    print("pers_img_time ime required = ", end_time - uv2coor_time)

    print("Total ime required = ", end_time - start_time)
    
    return pers_img
