from typing import Any
from SCons.Defaults import DefaultEnvironment
from SCons.Environment import Environment


def PhonyTargets(env: Environment | None = None, **kwargs: Any):
    if env is None:
        env = DefaultEnvironment()

    for target, action in kwargs.items():
        if isinstance(action, list):
            action = " ".join(action)
        env.AlwaysBuild(
            env.Alias(target, [], action)
        )  # make scons always run the command after building
