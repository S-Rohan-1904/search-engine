"""`search stem-step 1c|2|3|4 <word>` -- the derivational suffix tables."""

NAME = "Porter steps 1c, 2, 3 and 4"
ORDER = 80

CASES = [
    # --- step 1c ---
    {
        "name": "1c: happy -> happi -- the y in happy becomes i so it can meet happiness",
        "argv": ["stem-step", "1c", "happy"],
        "stdout": "happi",
    },
    {
        "name": "1c: sky -> sky -- sky keeps its y -- sk has no vowel",
        "argv": ["stem-step", "1c", "sky"],
        "stdout": "sky",
    },
    {
        "name": "1c: cry -> cry -- cry keeps its y for the same reason",
        "argv": ["stem-step", "1c", "cry"],
        "stdout": "cry",
    },
    {
        "name": "1c: enjoy -> enjoi -- a y after a vowel still converts",
        "argv": ["stem-step", "1c", "enjoy"],
        "stdout": "enjoi",
    },
    {
        "name": "1c: y -> y -- a bare y has no stem at all",
        "argv": ["stem-step", "1c", "y"],
        "stdout": "y",
    },
    {
        "name": "1c: cat -> cat -- a word without a trailing y is untouched",
        "argv": ["stem-step", "1c", "cat"],
        "stdout": "cat",
    },
    # --- step 2 ---
    {
        "name": "2: relational -> relate -- ational becomes ate",
        "argv": ["stem-step", "2", "relational"],
        "stdout": "relate",
    },
    {
        "name": "2: conditional -> condition -- tional becomes tion",
        "argv": ["stem-step", "2", "conditional"],
        "stdout": "condition",
    },
    {
        "name": "2: rational -> rational -- tional is blocked: ra has m = 0",
        "argv": ["stem-step", "2", "rational"],
        "stdout": "rational",
    },
    {
        "name": "2: valenci -> valence -- enci becomes ence",
        "argv": ["stem-step", "2", "valenci"],
        "stdout": "valence",
    },
    {
        "name": "2: hesitanci -> hesitance -- anci becomes ance",
        "argv": ["stem-step", "2", "hesitanci"],
        "stdout": "hesitance",
    },
    {
        "name": "2: digitizer -> digitize -- izer becomes ize",
        "argv": ["stem-step", "2", "digitizer"],
        "stdout": "digitize",
    },
    {
        "name": "2: conformabli -> conformable -- abli becomes able",
        "argv": ["stem-step", "2", "conformabli"],
        "stdout": "conformable",
    },
    {
        "name": "2: radicalli -> radical -- alli becomes al",
        "argv": ["stem-step", "2", "radicalli"],
        "stdout": "radical",
    },
    {
        "name": "2: differentli -> different -- entli becomes ent",
        "argv": ["stem-step", "2", "differentli"],
        "stdout": "different",
    },
    {
        "name": "2: vileli -> vile -- eli becomes e",
        "argv": ["stem-step", "2", "vileli"],
        "stdout": "vile",
    },
    {
        "name": "2: analogousli -> analogous -- ousli becomes ous",
        "argv": ["stem-step", "2", "analogousli"],
        "stdout": "analogous",
    },
    {
        "name": "2: vietnamization -> vietnamize -- ization becomes ize, not ation becomes ate",
        "argv": ["stem-step", "2", "vietnamization"],
        "stdout": "vietnamize",
    },
    {
        "name": "2: predication -> predicate -- ation becomes ate",
        "argv": ["stem-step", "2", "predication"],
        "stdout": "predicate",
    },
    {
        "name": "2: operator -> operate -- ator becomes ate",
        "argv": ["stem-step", "2", "operator"],
        "stdout": "operate",
    },
    {
        "name": "2: feudalism -> feudal -- alism becomes al",
        "argv": ["stem-step", "2", "feudalism"],
        "stdout": "feudal",
    },
    {
        "name": "2: decisiveness -> decisive -- iveness becomes ive",
        "argv": ["stem-step", "2", "decisiveness"],
        "stdout": "decisive",
    },
    {
        "name": "2: hopefulness -> hopeful -- fulness becomes ful",
        "argv": ["stem-step", "2", "hopefulness"],
        "stdout": "hopeful",
    },
    {
        "name": "2: callousness -> callous -- ousness becomes ous",
        "argv": ["stem-step", "2", "callousness"],
        "stdout": "callous",
    },
    {
        "name": "2: formaliti -> formal -- aliti becomes al",
        "argv": ["stem-step", "2", "formaliti"],
        "stdout": "formal",
    },
    {
        "name": "2: sensitiviti -> sensitive -- iviti becomes ive",
        "argv": ["stem-step", "2", "sensitiviti"],
        "stdout": "sensitive",
    },
    {
        "name": "2: sensibiliti -> sensible -- biliti becomes ble",
        "argv": ["stem-step", "2", "sensibiliti"],
        "stdout": "sensible",
    },
    {
        "name": "2: cat -> cat -- a word matching no rule is untouched",
        "argv": ["stem-step", "2", "cat"],
        "stdout": "cat",
    },
    # --- step 3 ---
    {
        "name": "3: triplicate -> triplic -- icate becomes ic",
        "argv": ["stem-step", "3", "triplicate"],
        "stdout": "triplic",
    },
    {
        "name": "3: ative -> ative -- ative is removed, but only when the stem has m > 0",
        "argv": ["stem-step", "3", "ative"],
        "stdout": "ative",
    },
    {
        "name": "3: formative -> form -- ative is removed outright",
        "argv": ["stem-step", "3", "formative"],
        "stdout": "form",
    },
    {
        "name": "3: formalize -> formal -- alize becomes al",
        "argv": ["stem-step", "3", "formalize"],
        "stdout": "formal",
    },
    {
        "name": "3: electriciti -> electric -- iciti becomes ic",
        "argv": ["stem-step", "3", "electriciti"],
        "stdout": "electric",
    },
    {
        "name": "3: electrical -> electric -- ical becomes ic",
        "argv": ["stem-step", "3", "electrical"],
        "stdout": "electric",
    },
    {
        "name": "3: hopeful -> hope -- ful is removed",
        "argv": ["stem-step", "3", "hopeful"],
        "stdout": "hope",
    },
    {
        "name": "3: goodness -> good -- ness is removed",
        "argv": ["stem-step", "3", "goodness"],
        "stdout": "good",
    },
    {
        "name": "3: cat -> cat -- a word matching no rule is untouched",
        "argv": ["stem-step", "3", "cat"],
        "stdout": "cat",
    },
    # --- step 4 ---
    {
        "name": "4: revival -> reviv -- al is removed",
        "argv": ["stem-step", "4", "revival"],
        "stdout": "reviv",
    },
    {
        "name": "4: allowance -> allow -- ance is removed",
        "argv": ["stem-step", "4", "allowance"],
        "stdout": "allow",
    },
    {
        "name": "4: inference -> infer -- ence is removed",
        "argv": ["stem-step", "4", "inference"],
        "stdout": "infer",
    },
    {
        "name": "4: airliner -> airlin -- er is removed",
        "argv": ["stem-step", "4", "airliner"],
        "stdout": "airlin",
    },
    {
        "name": "4: gyroscopic -> gyroscop -- ic is removed",
        "argv": ["stem-step", "4", "gyroscopic"],
        "stdout": "gyroscop",
    },
    {
        "name": "4: adjustable -> adjust -- able is removed",
        "argv": ["stem-step", "4", "adjustable"],
        "stdout": "adjust",
    },
    {
        "name": "4: defensible -> defens -- ible is removed",
        "argv": ["stem-step", "4", "defensible"],
        "stdout": "defens",
    },
    {
        "name": "4: irritant -> irrit -- ant is removed",
        "argv": ["stem-step", "4", "irritant"],
        "stdout": "irrit",
    },
    {
        "name": "4: replacement -> replac -- ement is removed, not just ment",
        "argv": ["stem-step", "4", "replacement"],
        "stdout": "replac",
    },
    {
        "name": "4: adjustment -> adjust -- ment is removed",
        "argv": ["stem-step", "4", "adjustment"],
        "stdout": "adjust",
    },
    {
        "name": "4: dependent -> depend -- ent is removed",
        "argv": ["stem-step", "4", "dependent"],
        "stdout": "depend",
    },
    {
        "name": "4: adoption -> adopt -- ion is removed because the stem ends in t",
        "argv": ["stem-step", "4", "adoption"],
        "stdout": "adopt",
    },
    {
        "name": "4: decision -> decis -- ion is removed because the stem ends in s",
        "argv": ["stem-step", "4", "decision"],
        "stdout": "decis",
    },
    {
        "name": "4: lion -> lion -- ion stays: the stem li ends in neither s nor t",
        "argv": ["stem-step", "4", "lion"],
        "stdout": "lion",
    },
    {
        "name": "4: homologou -> homolog -- ou is removed",
        "argv": ["stem-step", "4", "homologou"],
        "stdout": "homolog",
    },
    {
        "name": "4: communism -> commun -- ism is removed",
        "argv": ["stem-step", "4", "communism"],
        "stdout": "commun",
    },
    {
        "name": "4: activate -> activ -- ate is removed",
        "argv": ["stem-step", "4", "activate"],
        "stdout": "activ",
    },
    {
        "name": "4: angulariti -> angular -- iti is removed",
        "argv": ["stem-step", "4", "angulariti"],
        "stdout": "angular",
    },
    {
        "name": "4: homologous -> homolog -- ous is removed",
        "argv": ["stem-step", "4", "homologous"],
        "stdout": "homolog",
    },
    {
        "name": "4: effective -> effect -- ive is removed",
        "argv": ["stem-step", "4", "effective"],
        "stdout": "effect",
    },
    {
        "name": "4: bowdlerize -> bowdler -- ize is removed",
        "argv": ["stem-step", "4", "bowdlerize"],
        "stdout": "bowdler",
    },
    {
        "name": "4: rival -> rival -- al stays: riv has m = 1, and this step wants m > 1",
        "argv": ["stem-step", "4", "rival"],
        "stdout": "rival",
    },
    {
        "name": "4: cat -> cat -- a word matching no suffix is untouched",
        "argv": ["stem-step", "4", "cat"],
        "stdout": "cat",
    },
    {
        "name": "an unknown step is rejected",
        "argv": ["stem-step", "9z", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
