"""`search stem-step 1a|1b <word>` -- the plural and past-tense rules."""

NAME = "Porter steps 1a and 1b"
ORDER = 70

CASES = [
    # --- step 1a ---
    {
        "name": "1a: caresses -> caress -- sses becomes ss",
        "argv": ["stem-step", "1a", "caresses"],
        "stdout": "caress",
    },
    {
        "name": "1a: ponies -> poni -- ies becomes i",
        "argv": ["stem-step", "1a", "ponies"],
        "stdout": "poni",
    },
    {
        "name": "1a: ties -> ti -- ies becomes i even on a short word",
        "argv": ["stem-step", "1a", "ties"],
        "stdout": "ti",
    },
    {
        "name": "1a: caress -> caress -- ss is left alone rather than stripped to a bare s",
        "argv": ["stem-step", "1a", "caress"],
        "stdout": "caress",
    },
    {
        "name": "1a: cats -> cat -- a plain trailing s is dropped",
        "argv": ["stem-step", "1a", "cats"],
        "stdout": "cat",
    },
    {
        "name": "1a: dogs -> dog -- a plain trailing s is dropped",
        "argv": ["stem-step", "1a", "dogs"],
        "stdout": "dog",
    },
    {
        "name": "1a: likes -> like -- es is not a rule, so only the s comes off",
        "argv": ["stem-step", "1a", "likes"],
        "stdout": "like",
    },
    {
        "name": "1a: flies -> fli -- ies wins over the bare s rule",
        "argv": ["stem-step", "1a", "flies"],
        "stdout": "fli",
    },
    {
        "name": "1a: studies -> studi -- ies wins over the bare s rule",
        "argv": ["stem-step", "1a", "studies"],
        "stdout": "studi",
    },
    {
        "name": "1a: cat -> cat -- a word with no plural suffix is untouched",
        "argv": ["stem-step", "1a", "cat"],
        "stdout": "cat",
    },
    {
        "name": "1a: gas -> ga -- Porter is crude: gas loses its s like any plural",
        "argv": ["stem-step", "1a", "gas"],
        "stdout": "ga",
    },
    # --- step 1b ---
    {
        "name": "1b: feed -> feed -- eed survives because only f is left underneath, m = 0",
        "argv": ["stem-step", "1b", "feed"],
        "stdout": "feed",
    },
    {
        "name": "1b: agreed -> agree -- eed becomes ee because agr has m = 1",
        "argv": ["stem-step", "1b", "agreed"],
        "stdout": "agree",
    },
    {
        "name": "1b: plastered -> plaster -- ed comes off a stem that has a vowel",
        "argv": ["stem-step", "1b", "plastered"],
        "stdout": "plaster",
    },
    {
        "name": "1b: bled -> bled -- ed stays because bl has no vowel",
        "argv": ["stem-step", "1b", "bled"],
        "stdout": "bled",
    },
    {
        "name": "1b: motoring -> motor -- ing comes off a stem that has a vowel",
        "argv": ["stem-step", "1b", "motoring"],
        "stdout": "motor",
    },
    {
        "name": "1b: sing -> sing -- ing stays because s has no vowel",
        "argv": ["stem-step", "1b", "sing"],
        "stdout": "sing",
    },
    {
        "name": "1b: conflated -> conflate -- cleanup turns conflat into conflate",
        "argv": ["stem-step", "1b", "conflated"],
        "stdout": "conflate",
    },
    {
        "name": "1b: troubled -> trouble -- cleanup turns troubl into trouble",
        "argv": ["stem-step", "1b", "troubled"],
        "stdout": "trouble",
    },
    {
        "name": "1b: sized -> size -- cleanup turns siz into size",
        "argv": ["stem-step", "1b", "sized"],
        "stdout": "size",
    },
    {
        "name": "1b: hopping -> hop -- a doubled consonant collapses to one",
        "argv": ["stem-step", "1b", "hopping"],
        "stdout": "hop",
    },
    {
        "name": "1b: tanned -> tan -- a doubled consonant collapses to one",
        "argv": ["stem-step", "1b", "tanned"],
        "stdout": "tan",
    },
    {
        "name": "1b: falling -> fall -- a doubled l is kept",
        "argv": ["stem-step", "1b", "falling"],
        "stdout": "fall",
    },
    {
        "name": "1b: hissing -> hiss -- a doubled s is kept",
        "argv": ["stem-step", "1b", "hissing"],
        "stdout": "hiss",
    },
    {
        "name": "1b: fizzed -> fizz -- a doubled z is kept",
        "argv": ["stem-step", "1b", "fizzed"],
        "stdout": "fizz",
    },
    {
        "name": "1b: failing -> fail -- fail has m = 1 but fails cvc, so no e is restored",
        "argv": ["stem-step", "1b", "failing"],
        "stdout": "fail",
    },
    {
        "name": "1b: filing -> file -- fil has m = 1 and matches cvc, so it regains its e",
        "argv": ["stem-step", "1b", "filing"],
        "stdout": "file",
    },
    {
        "name": "1b: running -> run -- a doubled n collapses to one",
        "argv": ["stem-step", "1b", "running"],
        "stdout": "run",
    },
    {
        "name": "1b: hunting -> hunt -- nothing in the cleanup applies",
        "argv": ["stem-step", "1b", "hunting"],
        "stdout": "hunt",
    },
    {
        "name": "1b: cooking -> cook -- nothing in the cleanup applies",
        "argv": ["stem-step", "1b", "cooking"],
        "stdout": "cook",
    },
    {
        "name": "1b: cat -> cat -- a word with neither suffix is untouched",
        "argv": ["stem-step", "1b", "cat"],
        "stdout": "cat",
    },
]
