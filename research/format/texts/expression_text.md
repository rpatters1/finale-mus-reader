# ExpressionText

**Covers:** The two construction paths for expression raw text.
**Read when:** Working on expression text or its owning definition.
**Confidence:** partial; see the owning definition's evidence and remaining scope.

Pooled expression text is recovered by `src/import/texts/text_pool.cpp`. Earlier inline text
is synthesized by the [TextExpressionDef importer](../others/text_expression_defs.md), which
owns its font, text conversion, and TextBlock construction rules.
Recovery-report comparison is currently deferred with the
[owning definition cluster](../others/text_expression_defs.md#coverage-and-remaining-work).

The previous deferral of early synthesis is superseded by that importer. The previous Coda
whole-word-size hypothesis is retained and revised in its
[investigation](../../investigations/text_expression_defs.md#revised-hypotheses).
