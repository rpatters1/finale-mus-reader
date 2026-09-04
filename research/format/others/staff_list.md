# Staff lists

**Covers:** Legacy recovery of `StaffListCategoryName`, `StaffListCategoryParts`, and
`StaffListCategoryScore`, plus the five `StaffListRepeat*` classes.
**Read when:** Changing `src/import/others/staff_list.cpp` or interpreting category or
repeat staff-list records.
**Confidence:** confirmed for category records and fallback and for the controlled repeat-list
layouts; category override contents remain open.

## Repeat staff lists

Repeat staff lists have one named-tag representation and one class-record representation. The
importer recognizes the named tags in every fixed-row epoch and the class ids in the zlib epoch.

| Component | Fixed-row tag | Zlib class | musxdom class |
|---|---:|---:|---|
| name | `Dc` | `0x00e1` | `StaffListRepeatName` |
| score | `DC` | `0x00e4` | `StaffListRepeatScore` |
| parts | `dc` | `0x00e2` | `StaffListRepeatParts` |
| forced score | `IO` | `0x00e5` | `StaffListRepeatScoreForced` |
| forced parts | `io` | `0x00e3` | `StaffListRepeatPartsForced` |

Every stored component is imported independently under its source cmper and part. Membership
payloads are signed 16-bit `StaffCmper` arrays. An array expands to as many six-word incidences as
it needs; fixed-row files store those as separate rows, while zlib class records can concatenate
the six-word chunks into one payload. Zero terminates the used values within each chunk, rather
than the whole array. Negative values are retained for musxdom to interpret. Forced arrays have
the same representation and are ordinary imported components. An all-zero component is empty and
creates no object. Repeat lists are source-owned: absence creates no object and never requests one
from the baseline.

Repeat-list names use the same platform-encoded byte representation as category-list names.
Finale 1.0 already stores repeat membership through `DC` and `dc`, but the controlled file stores
no name; its Finale 27 companion preserves the memberships and adds a default list name. Files
through the Finale 3.7.2 generation appear to predate the UI for assigning names, while later
documents can also omit them. The importer therefore does not infer a name from either age or
membership.

The distinct `StaffAssignStaffList` selector family (`Ss`, `sp`, `So`, `so`, and `Sl`) has not
occurred as named records in the controlled evidence. It may be per-document UI state in Finale
2009 and later, but that interpretation remains open. It is intentionally ignored until expression
assignment recovery supplies evidence that it affects document semantics.

The controlled selector evidence and the finding that list presence does not imply a selected
document repeat list are recorded in the
[`RepeatOptions` investigation](../../investigations/repeat_options.md).

## Marking-category staff lists

The three components share a cmper in the range 1–8. Class `0x012f` supplies the optional name.
The legacy selectors call class `0x0130` the score list and `0x0132` the parts list, but their
contents map respectively to modern `StaffListCategoryParts` and `StaffListCategoryScore`.
Payloads contain up to six signed 16-bit `StaffCmper` values followed by zero fill. Values `-1`
and `-2` retain musxdom's top-staff and bottom-staff sentinel meanings.

Names are variable-length one-byte character sequences. Their payload is already in character
order, independent of the container's numeric byte order. Every legacy MUS saving version,
including Finale 2012, stores these bytes in the source platform encoding; the reader converts them
to UTF-8 through the shared platform-code-page path.

Category staff lists first occur in Finale 2009. The reader tracks the cmpers for which it imports
any stored component, then asks musxdom to import each untracked cmper from 1–8 from the baseline.
Musxdom refuses an occupied cmper atomically. The pinned Finale 27 baseline supplies each accepted
score/parts list with the top-staff sentinel. This covers early Finale 2009 files that contain only
four lists, later eight-list files, selectively removed components, and pre-F2009 files with no
category-list records. The baseline has no category-list name objects, so it does not invent names.

Classes `0x0131` and `0x0133` are the score and parts override selectors. Finale's category setup
prevents creating these forced lists, and none has yet appeared in the surveyed material. The
reader recognizes their presence as evidence of the category-list record family, emits an Info
diagnostic for every instance, and leaves their contents unmapped.

Category evidence and the remaining override question are recorded in
[the investigation](../../investigations/staff_list.md).
