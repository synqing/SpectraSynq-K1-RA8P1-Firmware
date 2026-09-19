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
            ('if (quiet < 0.90F) {\n    decay_rate += kWaveformK1SilenceDecay * (1.0F - quiet);\n  }',
             'if (!present && !silent) {\n    decay_rate = 0.15F;\n  } else if (silent && quiet < 0.90F) {\n    decay_rate += kWaveformK1SilenceDecay * (1.0F - quiet);\n  }'),
            ('transportOutward(\n      channel.frame(), channel.previousFrame(),\n'
             '      kWaveformK1ScrollPixelsPerSecond / kNominalFramesPerSecond,\n'
             '      nominal_retention, dt);',
             'const bool travel = present && (peak > 0.25F || vu > 0.25F ||\n'
             '                         channel.focusedAudio().chroma_strength > 0.08F);\n'
             '  transportOutward(\n      channel.frame(), channel.previousFrame(),\n'
             '      travel ? kWaveformK1ScrollPixelsPerSecond / kNominalFramesPerSecond\n'
             '             : 0.0F,\n'
             '      nominal_retention, dt);'),
            ('const Pixel8 raw_colour = injectionColour(channel);\n'
             '  const float colour_alpha = exponentialAlpha(dt, kWaveformK1ColourTau);',
             'Pixel8 raw_colour = injectionColour(channel);\n'
             '  if (channel.focusedAudio().chroma_strength < 0.08F) {\n'
             '    const float fallback = peak > vu ? peak : vu;\n'
             '    raw_colour = explicitPaletteColour(channel.controls(), 0.0F,\n'
             '                                       fallback);\n'
             '  }\n'
             '  const float colour_alpha = exponentialAlpha(dt, kWaveformK1ColourTau);'),
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
