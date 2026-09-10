"""Explicit optional derivative of staged VP sources; pinned imports stay intact."""
import hashlib

def apply_palette_overlay(root):
    changes = {
        'core/visual/channel_render_state.h': [
            ('#include <cstdint>', '#include <cstdint>\n#include "palette_transition.h"'),
            ('struct ChannelVisualControls final {',
             'struct ChannelVisualControls final {\n  const PaletteTransition* palette_transition = nullptr;'),
        ],
        'core/visual/product_effect_renderer.cpp': [
            ('float brightestStopPhase(const ProductPaletteDescriptor& palette) noexcept {',
             'PaletteLinearRgb selectedPalette(const ChannelVisualControls& controls,\n'
             '                                 float phase, float level) noexcept {\n'
             '  return controls.palette_transition\n'
             '      ? controls.palette_transition->hd(phase, level)\n'
             '      : sampleProductPaletteHd(controls.palette_id, phase, level);\n}\n\n'
             'float brightestStopPhase(const ProductPaletteDescriptor& palette) noexcept {'),
            ('float delta = brightestStopPhase(palette) - phase;',
             'float delta = (controls.palette_transition\n'
             '        ? controls.palette_transition->brightestPhase()\n'
             '        : brightestStopPhase(palette)) - phase;'),
            ('sampleProductPaletteHd(\n      controls.palette_id, phase,',
             'selectedPalette(\n      controls, phase,'),
            ('sampleProductPaletteHd(controls.palette_id, wrap01(phase), level)',
             'selectedPalette(controls, wrap01(phase), level)'),
        ],
    }
    prepared = {}
    for name, substitutions in changes.items():
        path = root / name
        before = path.read_text()
        after = before
        for old, new in substitutions:
            if after.count(old) != 1:
                raise RuntimeError('palette overlay source mismatch: ' + name)
            after = after.replace(old, new)
        prepared[name] = (before, after)
    receipt = {}
    for name, (before, after) in prepared.items():
        (root / name).write_text(after)
        receipt[name] = {'before': hashlib.sha256(before.encode()).hexdigest(),
                         'after': hashlib.sha256(after.encode()).hexdigest()}
    return receipt
