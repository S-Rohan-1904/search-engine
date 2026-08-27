"""`search stem-step 5a|5b <word>` and `search stem <word>` -- the full algorithm."""

NAME = "Porter step 5 and the full stemmer"
ORDER = 90

CASES = [
    # --- step 5a ---
    {
        "name": "5a: probate -> probat -- the e goes: probat has m = 2",
        "argv": ["stem-step", "5a", "probate"],
        "stdout": "probat",
    },
    {
        "name": "5a: rate -> rate -- the e stays: rat has m = 1 and ends cvc",
        "argv": ["stem-step", "5a", "rate"],
        "stdout": "rate",
    },
    {
        "name": "5a: cease -> ceas -- the e goes: ceas has m = 1 but does not end cvc",
        "argv": ["stem-step", "5a", "cease"],
        "stdout": "ceas",
    },
    {
        "name": "5a: argue -> argu -- argu has m = 1 and ends in a vowel, so the e goes",
        "argv": ["stem-step", "5a", "argue"],
        "stdout": "argu",
    },
    {
        "name": "5a: hope -> hope -- hop has m = 1 and ends cvc, so the e stays",
        "argv": ["stem-step", "5a", "hope"],
        "stdout": "hope",
    },
    {
        "name": "5a: cat -> cat -- a word without a trailing e is untouched",
        "argv": ["stem-step", "5a", "cat"],
        "stdout": "cat",
    },
    # --- step 5b ---
    {
        "name": "5b: controll -> control -- a doubled l collapses at m > 1",
        "argv": ["stem-step", "5b", "controll"],
        "stdout": "control",
    },
    {
        "name": "5b: roll -> roll -- roll has m = 1, so both l's stay",
        "argv": ["stem-step", "5b", "roll"],
        "stdout": "roll",
    },
    {
        "name": "5b: fall -> fall -- fall has m = 1 too",
        "argv": ["stem-step", "5b", "fall"],
        "stdout": "fall",
    },
    {
        "name": "5b: control -> control -- a single l is left alone",
        "argv": ["stem-step", "5b", "control"],
        "stdout": "control",
    },
    # --- the full chain ---
    {
        "name": "stem: caresses -> caress -- the plural rule alone",
        "argv": ["stem", "caresses"],
        "stdout": "caress",
    },
    {
        "name": "stem: ponies -> poni -- 1a leaves poni, and nothing after it applies",
        "argv": ["stem", "ponies"],
        "stdout": "poni",
    },
    {
        "name": "stem: connect -> connect -- already a stem",
        "argv": ["stem", "connect"],
        "stdout": "connect",
    },
    {
        "name": "stem: connected -> connect -- past tense",
        "argv": ["stem", "connected"],
        "stdout": "connect",
    },
    {
        "name": "stem: connecting -> connect -- progressive",
        "argv": ["stem", "connecting"],
        "stdout": "connect",
    },
    {
        "name": "stem: connection -> connect -- the noun, via step 4's ion rule",
        "argv": ["stem", "connection"],
        "stdout": "connect",
    },
    {
        "name": "stem: connections -> connect -- plural noun, so 1a runs before step 4 ever sees it",
        "argv": ["stem", "connections"],
        "stdout": "connect",
    },
    {
        "name": "stem: argue -> argu -- five spellings of argue all meet at argu",
        "argv": ["stem", "argue"],
        "stdout": "argu",
    },
    {
        "name": "stem: argued -> argu -- five spellings of argue all meet at argu",
        "argv": ["stem", "argued"],
        "stdout": "argu",
    },
    {
        "name": "stem: argues -> argu -- five spellings of argue all meet at argu",
        "argv": ["stem", "argues"],
        "stdout": "argu",
    },
    {
        "name": "stem: arguing -> argu -- five spellings of argue all meet at argu",
        "argv": ["stem", "arguing"],
        "stdout": "argu",
    },
    {
        "name": "stem: argus -> argu -- five spellings of argue all meet at argu",
        "argv": ["stem", "argus"],
        "stdout": "argu",
    },
    {
        "name": "stem: controlling -> control -- 1b drops ing and undoes the doubling",
        "argv": ["stem", "controlling"],
        "stdout": "control",
    },
    {
        "name": "stem: controller -> control -- step 4 drops er, leaving controll, and 5b collapses it",
        "argv": ["stem", "controller"],
        "stdout": "control",
    },
    {
        "name": "stem: happy -> happi -- 1c converts the y",
        "argv": ["stem", "happy"],
        "stdout": "happi",
    },
    {
        "name": "stem: happiness -> happi -- step 3 drops ness and lands on the same key as happy",
        "argv": ["stem", "happiness"],
        "stdout": "happi",
    },
    {
        "name": "stem: generalizations -> gener -- every step in the chain fires",
        "argv": ["stem", "generalizations"],
        "stdout": "gener",
    },
    {
        "name": "stem: relational -> relat -- 2 maps ational to ate, then 5a drops the e",
        "argv": ["stem", "relational"],
        "stdout": "relat",
    },
    {
        "name": "stem: rational -> ration -- the same suffix, blocked by measure, so only 4 applies",
        "argv": ["stem", "rational"],
        "stdout": "ration",
    },
    {
        "name": "stem: conditional -> condit -- 2 maps tional to tion, then 4 removes ion",
        "argv": ["stem", "conditional"],
        "stdout": "condit",
    },
    {
        "name": "stem: meetings -> meet -- 1a then 1b then 4",
        "argv": ["stem", "meetings"],
        "stdout": "meet",
    },
    {
        "name": "stem: sky -> sky -- a short word the y rule leaves alone",
        "argv": ["stem", "sky"],
        "stdout": "sky",
    },
    {
        "name": "stem: is -> is -- two letters, returned untouched",
        "argv": ["stem", "is"],
        "stdout": "is",
    },
    {
        "name": "stem: a -> a -- one letter, returned untouched",
        "argv": ["stem", "a"],
        "stdout": "a",
    },
    {
        "name": "stem: cat -> cat -- nothing applies",
        "argv": ["stem", "cat"],
        "stdout": "cat",
    },
    {
        "name": "stem with no argument is rejected",
        "argv": ["stem"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
