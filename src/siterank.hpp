#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "pagerank.hpp"

// PageRank over sites rather than pages, credited to each site's home page.
//
// Page-level PageRank on a crawl mostly measures how a site links to itself:
// navigation bars, footers and "related" lists give every page of a large site
// hundreds of votes from its own pages. Those votes say nothing about whether
// anyone else thinks the site is worth reading. Collapsing the graph to one
// node per host, and keeping only links between different hosts, leaves the
// votes that do: one site's opinion of another.
//
// The construction:
//
//   1. map every page to its host, from its URL;
//   2. keep each link whose two ends are on different hosts, once per pair of
//      hosts however many pages carry it;
//   3. run `pagerank()` over the hosts;
//   4. give each host's score to its home page, the page whose path is "/"
//      and which has no query string. Every other page gets nothing.
//
// Crediting the home page is the point. A site's authority is evidence about
// the site, and the page a query naming the site should find is its front
// door, not whichever inner page happened to be crawled.
//
// A host reachable over both http and https has two home pages in a crawl; the
// one listed first in `urls`, which is the one the crawler reached first, is
// the one credited. A host with no crawled home page, and a page whose URL is
// missing or does not parse, contributes its links to the host graph where it
// can but receives no score.
//
// `links` are (from page, to page) pairs and `urls` are (page, URL) pairs, in
// the order a crawl writes them to links.tsv and urls.tsv. Returns the home
// pages of every host in the host graph that has one, highest score first,
// each with its host's score.
std::vector<RankedPage> site_pagerank(const std::vector<std::pair<std::string, std::string>>& links,
                                      const std::vector<std::pair<std::string, std::string>>& urls,
                                      std::size_t iterations, double damping);

// Every home page in a crawl with the same score, 1 / (number of home pages):
// the prior site-level PageRank has to beat. It favours a page for being a
// site's front door and for nothing else, using no links at all. On the
// 1,096-site crawl it found home pages better than site-level PageRank did,
// which is how most of that gain turned out to be this.
//
// Home pages are chosen as in `site_pagerank()`: path "/", no query string,
// the first listed when a host has two. Ordered by page name.
std::vector<RankedPage> home_page_ranking(
    const std::vector<std::pair<std::string, std::string>>& urls);

// Reads a crawl's urls.tsv: one page and its URL per line, tab separated, in
// file order.
bool read_page_urls(const std::filesystem::path& path,
                    std::vector<std::pair<std::string, std::string>>& urls, std::string& error);
