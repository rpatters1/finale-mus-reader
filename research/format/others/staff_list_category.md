# Marking-category staff lists

**Covers:** Legacy recovery of `StaffListCategoryName`, `StaffListCategoryParts`, and
`StaffListCategoryScore`.
**Read when:** Changing `src/import/others/staff_list_category.cpp` or interpreting classes
`0x012f` through `0x0133`.
**Confidence:** confirmed for the implemented F2009–F2012 records and pre-F2009 fallback;
override contents remain open.

The three components share a cmper in the range 1–8. Class `0x012f` supplies the optional name.
The legacy selectors call class `0x0130` the score list and `0x0132` the parts list, but their
contents map respectively to modern `StaffListCategoryParts` and `StaffListCategoryScore`.
Payloads contain up to six signed 16-bit `StaffCmper` values followed by zero fill. Values `-1`
and `-2` retain musxdom's top-staff and bottom-staff sentinel meanings.

Names are variable-length one-byte character sequences packed into payload words. Big-endian
containers therefore need the two bytes of each payload word restored to character order before
conversion. Every legacy MUS saving version, including Finale 2012, stores these bytes in the
source platform encoding; the reader converts them to UTF-8 through the shared platform-code-page
path.

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

Evidence and the remaining override question are recorded in
[the investigation](../../investigations/staff_list_category.md).
