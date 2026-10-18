// Evidence-based pseudocode, not a linked replacement or complete class recovery.
// Sources: outputs/overlay-{insertion-ripple,timing-order,timing-attribute}-recovery.json.
// All timing calculations retain x86 32-bit wraparound; an eventual C++ port must
// implement these through unsigned arithmetic instead of signed overflow.
// Equal-start ordering, invalid elements and the sort implementation need parity work.
//
// GetStart(clip) [104bba30]:
//   frame = Resolve(clip.timeFrame);
//   return frame ? frame.start : 0;
// GetEnd(clip) [104bba80]:
//   frame = Resolve(clip.timeFrame);
//   return frame ? WrapAdd32(frame.start, frame.duration) : 0;
//
// FindInsertionConflict(track, start, end) [104d3810]:
//   if track.type != 3 || track.count == 0: return null;
//   EnsureOrderedByStart(track);
//   for each handle in track.children:
//     clip = ResolveAndCheckType(handle);
//     if !clip: continue;
//     if start < GetEnd(clip) && end >= GetStart(clip): return clip;
//     if end <= GetStart(clip): return null;
//   return null;
//
// MakeInsertionRoom(track, start, end) [104d3ef0]:
//   if track.type != 3 || track.count == 0: return;
//   EnsureOrderedByStart(track);
//   clip = FindInsertionConflict(track, start, end);
//   if clip:
//     ShiftFromTarget(track, clip, WrapSubtract32(end, GetStart(clip)), true, true);
//
// ShiftFromTarget(track, target, delta, includeTarget, ensureOrder) [104d3d60]:
//   if track.type != 3 || track.count == 0 || delta == 0: return;
//   if ensureOrder: EnsureOrderedByStart(track);
//   for each handle in reverse(track.children):
//     clip = ResolveAndCheckType(handle);
//     if clip == target:
//       if includeTarget: SetStart(clip, WrapAdd32(GetStart(clip), delta));
//       return;
//     SetStart(clip, WrapAdd32(GetStart(clip), delta));
//   // Original path can pass null to timing accessors after failed type resolution.
//   // Do not invent a skip or exception policy without further evidence.

// SetOverlayMaterial(filmClip, selectedName) [104bf960]:
//   if selectedName && selectedName[0]:
//     effect = Resolve(filmClip.materialOverlay);
//     if !effect:
//       effect = CreateTyped<CDmeMaterialOverlayFXClip>("materialOverlay", filmClip.fileId, 0);
//       AssignElementReference(filmClip.materialOverlay, effect);
//     SetEffectMaterial(effect, selectedName); //105dcf90: material attribute
//   else:
//     AssignElementReferenceWord(filmClip.materialOverlay, 0xffffffff);
// Underlying data-model string conversion and error handling remain unresolved.
