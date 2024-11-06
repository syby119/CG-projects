import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection, Line3DCollection
from matplotlib.widgets import Slider
import numpy as np
import glm


fig = plt.figure()
ax1 = fig.add_subplot(122, projection='3d')
ax2 = fig.add_subplot(121, projection='3d')

px, py, pz = 0.0, 0.0, -20.0

axcolor = 'lightgoldenrodyellow'
ax_px = plt.axes([0.25, 0.15, 0.65, 0.03], facecolor=axcolor)
ax_py = plt.axes([0.25, 0.10, 0.65, 0.03], facecolor=axcolor)
ax_pz = plt.axes([0.25, 0.05, 0.65, 0.03], facecolor=axcolor)

slider_x = Slider(ax_px, 'Point X', -100, 100, valinit=px)
slider_y = Slider(ax_py, 'Point Y', -100, 100, valinit=py)
slider_z = Slider(ax_pz, 'Point Z', -75, 75, valinit=pz)

projection = glm.perspectiveFov(glm.radians(60), 1280, 720, 10, 50)


def draw_cube(ax):
    vertices = np.array([
        [-1.0, -1.0, -1.0],
        [ 1.0, -1.0, -1.0],
        [ 1.0,  1.0, -1.0],
        [-1.0,  1.0, -1.0],
        [-1.0, -1.0,  1.0],
        [ 1.0, -1.0,  1.0],
        [ 1.0,  1.0,  1.0],
        [-1.0,  1.0,  1.0]
    ])

    edges = [
        [vertices[0], vertices[1]],
        [vertices[1], vertices[2]],
        [vertices[2], vertices[3]],
        [vertices[3], vertices[0]],
        [vertices[4], vertices[5]],
        [vertices[5], vertices[6]],
        [vertices[6], vertices[7]],
        [vertices[7], vertices[4]],
        [vertices[0], vertices[4]],
        [vertices[1], vertices[5]],
        [vertices[2], vertices[6]],
        [vertices[3], vertices[7]]
    ]

    line_collection = Line3DCollection(edges, colors='k', linewidths=1)
    ax.add_collection3d(line_collection)


def draw_point_ndc(ax, p: glm.vec3):
    clip = projection * glm.vec4(p, 1.0)
    clip /= clip.w
    color = 'green' if abs(clip.x) <= 1 and abs(clip.y) <= 1 and abs(clip.z) <= 1 else 'red'
    ax.scatter(clip.x, clip.y, clip.z, color=color, marker='o')


def drawCoordinateSystem(ax, scale = 1):
    origin = np.array([[0, 0, 0]])
    x_dir = np.array([[scale, 0, 0]])
    y_dir = np.array([[0, scale, 0]])
    z_dir = np.array([[0, 0, scale]])

    ax.quiver(origin[:, 0], origin[:, 1], origin[:, 2],
            x_dir[:, 0], x_dir[:, 1], x_dir[:, 2], color='r', length=scale, normalize=True)
    ax.quiver(origin[:, 0], origin[:, 1], origin[:, 2],
            y_dir[:, 0], y_dir[:, 1], y_dir[:, 2], color='g', length=scale, normalize=True)
    ax.quiver(origin[:, 0], origin[:, 1], origin[:, 2],
            z_dir[:, 0], z_dir[:, 1], z_dir[:, 2], color='b', length=scale, normalize=True)


def getCullBits(positionCS: glm.vec4):
    cullBits = 0
    cullBits |= 1  if positionCS.x < -positionCS.w else 0
    cullBits |= 2  if positionCS.x >  positionCS.w else 0
    cullBits |= 4  if positionCS.y < -positionCS.w else 0
    cullBits |= 8  if positionCS.y >  positionCS.w else 0
    cullBits |= 16 if positionCS.z < -positionCS.w else 0
    cullBits |= 32 if positionCS.z >  positionCS.w else 0
    cullBits |= 64 if positionCS.w <= 0            else 0

    return cullBits


def draw_point(ax, p: glm.vec3):
    cullBits = getCullBits(projection * glm.vec4(p, 1.0))
    color = 'red' if cullBits else 'green'
    ax.scatter(p.x, p.y, p.z, color=color, marker='o')


def drawFrustum(ax):
    invProj = glm.inverse(projection)

    verticeNDCs = [
        glm.vec3(-1.0, -1.0, -1.0),
        glm.vec3(+1.0, -1.0, -1.0),
        glm.vec3(-1.0, +1.0, -1.0),
        glm.vec3(+1.0, +1.0, -1.0),
        glm.vec3(-1.0, -1.0, +1.0),
        glm.vec3(+1.0, -1.0, +1.0),
        glm.vec3(-1.0, +1.0, +1.0),
        glm.vec3(+1.0, +1.0, +1.0)
    ]

    vertices = []
    for i in range(8):
        p = invProj * glm.vec4(verticeNDCs[i], 1.0)
        p /= p.w
        vertices.append([p.x, p.y, p.z])

    vertices = np.array(vertices)

    edges = [
        [vertices[0], vertices[1]],
        [vertices[0], vertices[2]],
        [vertices[1], vertices[3]],
        [vertices[2], vertices[3]],
        [vertices[0], vertices[4]],
        [vertices[1], vertices[5]],
        [vertices[2], vertices[6]],
        [vertices[3], vertices[7]],
        [vertices[4], vertices[5]],
        [vertices[4], vertices[6]],
        [vertices[5], vertices[7]],
        [vertices[6], vertices[7]]
    ]

    line_collection = Line3DCollection(edges, colors='k', linewidths=1)
    ax.add_collection3d(line_collection)


def update(val):
    px, py, pz = slider_x.val, slider_y.val, slider_z.val
    ax1.cla()

    drawCoordinateSystem(ax1)
    draw_cube(ax1)
    draw_point_ndc(ax1, glm.vec3(px, py, pz))

    ax1.set_xlim([-2, 2])
    ax1.set_ylim([-2, 2])
    ax1.set_zlim([-2, 2])
    ax1.set_xlabel('X')
    ax1.set_ylabel('Y')
    ax1.set_zlabel('Z')
    ax1.set_box_aspect([1, 1, 1])

    ax1.set_title("NDC")

    ax2.cla()
    drawCoordinateSystem(ax2, 25)
    drawFrustum(ax2)
    draw_point(ax2, glm.vec3(px, py, pz))

    ax2.set_xlim([-100, 100])
    ax2.set_ylim([-100, 100])
    ax2.set_zlim([-100, 100])
    ax2.set_xlabel('X')
    ax2.set_ylabel('Y')
    ax2.set_zlabel('Z')
    ax2.set_box_aspect([1, 1, 1])

    ax2.set_title("View Space")

    plt.tight_layout()

    positionCS = projection * glm.vec4(px, py, pz, 1)
    cullBits = getCullBits(positionCS)
    fig.suptitle("Position CS: ({1:.3f}, {2:.3f}, {3:.3f}, {4:.3f})\nCull Bits: {0:07b} ".format(
        cullBits, positionCS.x, positionCS.y, positionCS.z, positionCS.w))

    plt.draw()


if __name__ == "__main__":
    update(0)

    slider_x.on_changed(update)
    slider_y.on_changed(update)
    slider_z.on_changed(update)

    plt.show()