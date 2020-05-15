from abc import ABC, abstractmethod
from hoomd.hoomd_array import HOOMDArray, HOOMDGPUArray
from hoomd import _hoomd


class _LocalAccess(ABC):
    @property
    @abstractmethod
    def _fields(self):
        pass

    @property
    @abstractmethod
    def _array_cls(self):
        pass

    _accessed_fields = dict()

    def __init__(self):
        object.__setattr__(self, '_entered', False)

    def __getattr__(self, attr):
        if attr in self._accessed_fields:
            return self._accessed_fields[attr]
        elif attr.startswith("ghost_"):
            if attr[6:] in self._fields:
                buff = getattr(self._cpp_obj, self._fields[attr[6:]])(
                    True, False)
            else:
                raise AttributeError(
                    "{} object has no attribute {}".format(type(self), attr))
        elif attr.endswith("_with_ghosts"):
            if attr[:-12] in self._fields:
                buff = getattr(self._cpp_obj, self._fields[attr[:-12]])(
                    False, True)
            else:
                raise AttributeError(
                    "{} object has no attribute {}".format(type(self), attr))
        elif attr in self._fields:
            buff = getattr(self._cpp_obj, self._fields[attr])(False, False)
        else:
            raise AttributeError(
                "{} object has no attribute {}".format(type(self), attr))
        arr = self._array_cls(buff, lambda: self._entered)
        self._accessed_fields[attr] = arr
        return arr

    def __setattr__(self, attr, value):
        try:
            arr = getattr(self, attr)
        except AttributeError:
            raise AttributeError(
                "Cannot set attribute {}, does not exist.".format(attr))
        if arr.read_only == True:
            raise RuntimeError(
                "Attribute {} is not settable.".format(attr))
        arr[:] = value

    def _enter(self):
        self._cpp_obj.enter()
        object.__setattr__(self, '_entered', True)

    def _exit(self):
        self._cpp_obj.exit()
        object.__setattr__(self, '_entered', False)
        object.__setattr__(self, '_accessed_fields', dict())


class _ParticleLocalAccess(_LocalAccess):
    @property
    @abstractmethod
    def _cpp_cls(self):
        pass

    _fields = {
        'position': 'getPosition',
        'type': 'getTypes',
        'velocity': 'getVelocities',
        'mass': 'getMasses',
        'acceleration': 'getAcceleration',
        'orientation': 'getOrientation',
        'angular_momentum': 'getAngularMomentum',
        'moment_of_inertia': 'getMomentsOfInertia',
        'charge': 'getCharge',
        'diameter': 'getDiameter',
        'image': 'getImages',
        'tag': 'getTags',
        'rtag': 'getRTags',
        'rigid_body_id': 'getBodies',
        'net_force': 'getNetForce',
        'net_torque': 'getNetTorque',
        'net_virial': 'getNetVirial'}

    def __init__(self, state):
        super().__init__()
        object.__setattr__(
            self, '_cpp_obj',
            self._cpp_cls(state._cpp_sys_def.getParticleData()))


class ParticleLocalAccessCPU(_ParticleLocalAccess):
    _cpp_cls = _hoomd.LocalParticleDataHost
    _array_cls = HOOMDArray


class ParticleLocalAccessGPU(_ParticleLocalAccess):
    _cpp_cls = _hoomd.LocalParticleDataDevice
    _array_cls = HOOMDGPUArray


class _LocalSnapshotBase:
    @property
    def particles(self):
        return self._particles

    def __enter__(self):
        self._particles._enter()
        return self

    def __exit__(self, type, value, traceback):
        self._particles._exit()


class LocalSnapshot(_LocalSnapshotBase):
    def __init__(self, state):
        self._particles = ParticleLocalAccessCPU(state)


class LocalSnapshotGPU(_LocalSnapshotBase):
    def __init__(self, state):
        self._particles = ParticleLocalAccessGPU(state)