"""Prepared entry: old closed ledger still refuses before runtime construction.

No approval is granted by importing or selecting this wrapper. The future source
manifest and explicit music-contract continuation approval are still required.
"""
import checkpoint_startup54 as original_wrapper
from stage56_stream_adapter import with_generic_music

ORIGINAL_FACTORY = original_wrapper.runtime_class


def music_factory(mode, watch_state, tag):
    Base,x,u,Boundary,base = ORIGINAL_FACTORY(mode,watch_state,tag)
    return with_generic_music(Base,x,Boundary),x,u,Boundary,base


if __name__ == '__main__':
    original_wrapper.runtime_class = music_factory
    raise SystemExit(original_wrapper.main())
