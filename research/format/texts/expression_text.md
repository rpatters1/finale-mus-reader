# ExpressionText

**Covers:** Where expression text lived before it moved into the text pool.
**Read when:** Working on expression text in the pre-pool eras.
**Confidence:** partial; pooled eras only.

## Expression text before it moved into the pool

**Confirmed** for the uncompressed epoch. Finale 2000 and earlier keep expression text inside
the text expression definition, in the `DT` family, one expression per comparator:

| Location | Field |
|---|---|
| incidence 0, byte 0 | point size |
| incidence 0, byte 1 | font definition comparator |
| incidence 0, word 1 | `nfx` style bits |
| incidence 1 onward | the display text, twelve bytes per row, ending at the first NUL |

**Recovering it is deferred until `TextExpressionDef` is imported.** The layout above is
established, and the reader once synthesized an Enigma string from it, but a text pool full of
expression strings with no definitions behind them claims more coverage than it has: the
definition is what gives the text its meaning. The synthesis is removed rather than switched
off, and a test asserts that these eras produce no `ExpressionText`, so reinstating it is a
deliberate act.

**The move happens inside the DCL epoch, not at it.** Finale 2002 still keeps display text in
`DT` under exactly the layout above — `F2002-fileinfo-text.mus` holds `ffff`, `pppp` and
`Tempo (=#)` there, matching its companion's expressions — and its text pool carries no
`^expression` record at all. By Finale 2006 the display text has moved to the pool and the string
embedded in `DT` is the expression's *description* instead, `Below Staff (Vel. 127)` and the
like. Reading `DT` as display text in that later range would fill the texts pool with category
descriptions.

The move is therefore bounded between Finale 2003 and Finale 2006 and is otherwise **open**. No
document of that range defines an expression.

The Coda-banner `DT` layout differs again — its size is a whole word rather than a packed
byte, and its text incidence carries further fields after the string — and is **open**.
