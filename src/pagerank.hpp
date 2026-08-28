#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// A page and its PageRank score.
struct RankedPage {
    std::string page;
    double score;
};

// PageRank over a link graph, by power iteration.
//
// The score of a page is the probability that a random surfer, following links
// and occasionally jumping to a page at random, is looking at it. A link is a
// vote, a vote from a page with few outgoing links counts for more, and a vote
// from a well-scored page counts for more still. The recursion is the point:
// importance is defined in terms of itself, and iterating to a fixed point is
// how it gets resolved.
//
//     score(p) = (1 - d) / N  +  d * sum over q linking to p of score(q) / out(q)
//
// `damping` is the probability the surfer follows a link rather than jumping,
// conventionally 0.85. Pages with no outgoing links are dangling: their score
// would leak out of the system every round, so it is redistributed evenly
// instead, which is the same as pretending they link to everything.
//
// A fixed iteration count rather than a convergence threshold, so the result is
// reproducible rather than dependent on floating-point noise near the limit.
// Thirty rounds is well past where the ordering stops changing on graphs this
// size.
//
// Repeated links between the same pair count once. A vote is a page's opinion
// that another page is worth reading, and saying it three times does not make
// it three opinions.
//
// Returns every page, highest score first, ties broken by name.
std::vector<RankedPage> pagerank(const std::vector<std::pair<std::string, std::string>>& links,
                                 std::size_t iterations, double damping);
