#include "server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "normalize.hpp"

namespace {

const char* const kStyle =
    "body{font:16px/1.5 system-ui,sans-serif;max-width:46rem;margin:2rem auto;padding:0 1rem;"
    "color:#1d1d1f;background:#fff}"
    "form{display:flex;gap:.5rem;margin-bottom:1.5rem}"
    ".box{position:relative;flex:1}"
    "input[name=q]{width:100%;box-sizing:border-box;font:inherit;padding:.5rem .75rem;"
    "border:1px solid #c7c7cc;border-radius:6px}"
    "#suggest{position:absolute;left:0;right:0;top:100%;margin:2px 0 0;padding:.25rem 0;"
    "list-style:none;background:#fff;border:1px solid #c7c7cc;border-radius:6px;"
    "box-shadow:0 4px 12px rgba(0,0,0,.08);z-index:1}"
    "#suggest[hidden]{display:none}"
    "#suggest li{margin:0;padding:.35rem .75rem;cursor:pointer;display:flex;"
    "justify-content:space-between;gap:1rem}"
    "#suggest li span{color:#6e6e73;font-size:.8rem}"
    "#suggest li[aria-selected=true]{background:#d4e4f7}"
    "button{font:inherit;padding:.5rem 1rem;border:0;border-radius:6px;background:#0a66c2;"
    "color:#fff}"
    "ol{padding-left:1.25rem}li{margin-bottom:1.25rem}"
    ".url{color:#1a7f37;font-size:.85rem;word-break:break-all}"
    ".score{color:#6e6e73;font-size:.8rem}"
    ".snippet{margin:.25rem 0 0}.note{color:#6e6e73}"
    "@media (prefers-color-scheme:dark){body{color:#f5f5f7;background:#1c1c1e}"
    "input[name=q]{background:#2c2c2e;color:#f5f5f7;border-color:#48484a}"
    "#suggest{background:#2c2c2e;border-color:#48484a}#suggest li span{color:#a1a1a6}"
    "#suggest li[aria-selected=true]{background:#1f3d63}"
    ".url{color:#4ac26b}.score,.note{color:#a1a1a6}a{color:#6cb6ff}}";

const char* const kScript =
    "(()=>{const input=document.querySelector('input[name=q]');"
    "const list=document.getElementById('suggest');let items=[];let active=-1;let timer=0;"
    "let token=0;"
    "const split=v=>{const m=v.match(/^(.*?)([^\\s()\"]*)$/);return [m[1],m[2]];};"
    "const close=()=>{list.hidden=true;active=-1;input.setAttribute('aria-expanded','false');"
    "input.removeAttribute('aria-activedescendant');};"
    "const mark=()=>{[...list.children].forEach((li,i)=>li.setAttribute('aria-selected',"
    "i===active?'true':'false'));if(active>=0){input.setAttribute('aria-activedescendant',"
    "'suggest-'+active);}else{input.removeAttribute('aria-activedescendant');}};"
    "const pick=i=>{const [head]=split(input.value);input.value=head+items[i].term+' ';"
    "close();input.focus();};"
    "const show=data=>{items=data.completions;list.replaceChildren();"
    "items.forEach((c,i)=>{const li=document.createElement('li');li.id='suggest-'+i;"
    "li.setAttribute('role','option');li.append(c.term);const n=document.createElement('span');"
    "n.textContent=c.documents+(c.documents===1?' doc':' docs');li.append(n);"
    "li.addEventListener('mousedown',e=>{e.preventDefault();pick(i);});list.append(li);});"
    "active=-1;mark();list.hidden=items.length===0;"
    "input.setAttribute('aria-expanded',items.length?'true':'false');};"
    "input.addEventListener('input',()=>{clearTimeout(timer);const [,word]=split(input.value);"
    "if(word.length<2){close();return;}const mine=++token;timer=setTimeout(()=>{"
    "fetch('/api/complete?limit=8&q='+encodeURIComponent(word)).then(r=>r.json())"
    ".then(d=>{if(mine===token){show(d);}}).catch(close);},120);});"
    "input.addEventListener('keydown',e=>{if(list.hidden){return;}"
    "if(e.key==='ArrowDown'){e.preventDefault();active=(active+1)%items.length;mark();}"
    "else if(e.key==='ArrowUp'){e.preventDefault();"
    "active=(active-1+items.length)%items.length;mark();}"
    "else if((e.key==='Enter'||e.key==='Tab')&&active>=0){e.preventDefault();pick(active);}"
    "else if(e.key==='Escape'){close();}});"
    "input.addEventListener('blur',()=>setTimeout(close,100));})();";

bool parse_weight(const std::string& text, double& out) {
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed != text.size() || !std::isfinite(value) || value < 0.0) {
            return false;
        }
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_limit(const std::string& text, std::size_t& out) {
    if (text.empty() || text.size() > 4) {
        return false;
    }
    std::size_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    if (value == 0 || value > 100) {
        return false;
    }
    out = value;
    return true;
}

bool parse_word_count(const std::string& text, std::size_t& out) {
    if (text.empty() || text.size() > 4) {
        return false;
    }
    std::size_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    out = value;
    return true;
}

std::string flatten(const std::string& text) {
    std::string out;
    bool space = false;
    for (const char c : text) {
        if (c == '\n' || c == '\r' || c == '\t') {
            space = true;
            continue;
        }
        if (space && !out.empty()) {
            out.push_back(' ');
        }
        space = false;
        out.push_back(c);
    }
    return out;
}

std::string page(const std::string& query, const std::string& content) {
    std::ostringstream out;
    out << "<!doctype html>\n<html lang=\"en\"><head><meta charset=\"utf-8\">"
        << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        << "<title>" << (query.empty() ? "Search" : html_escape(query) + " - Search")
        << "</title><style>" << kStyle << "</style></head><body>"
        << "<form action=\"/search\" method=\"get\" role=\"search\">"
        << "<div class=\"box\"><input name=\"q\" value=\"" << html_escape(query)
        << "\" aria-label=\"Query\" autocomplete=\"off\" role=\"combobox\""
        << " aria-autocomplete=\"list\" aria-controls=\"suggest\" aria-expanded=\"false\""
        << " autofocus><ul id=\"suggest\" role=\"listbox\" hidden></ul></div>"
        << "<button type=\"submit\">Search</button></form>" << content << "<script>" << kScript
        << "</script></body></html>\n";
    return out.str();
}

}

SearchService::SearchService(const InvertedIndex& index, const CorpusReader* corpus,
                             const PriorSet& priors,
                             std::unordered_map<std::string, std::string> urls,
                             QueryOptions defaults)
    : index_(index),
      corpus_(corpus),
      priors_(priors),
      urls_(std::move(urls)),
      defaults_(std::move(defaults)) {
    defaults_.snippets = corpus_ != nullptr;
    defaults_.tsv = false;
}

const BkTree& SearchService::tree() {
    if (!tree_.has_value()) {
        tree_ = build_bk_tree(index_);
    }
    return *tree_;
}

void SearchService::warm() {
    tree();
    trie();
}

const Trie& SearchService::trie() {
    if (!trie_.has_value()) {
        trie_ = build_trie(index_);
    }
    return *trie_;
}

HttpResponse SearchService::completions(const HttpRequest& request) {
    const auto q = request.query.find("q");
    const std::string prefix = q == request.query.end() ? std::string() : q->second;

    std::size_t limit = 8;
    const auto asked = request.query.find("limit");
    if (asked != request.query.end() && !parse_limit(asked->second, limit)) {
        return HttpResponse{400, "application/json",
                            "{\"error\":\"limit must be a whole number from 1 to 100\"}\n"};
    }

    const std::string term = normalize(prefix);
    std::ostringstream out;
    out << "{\"prefix\":\"" << json_escape(term) << "\",\"completions\":[";
    if (!term.empty()) {
        bool first = true;
        for (const Completion& completion : trie().complete(term, limit)) {
            out << (first ? "" : ",") << "{\"term\":\"" << json_escape(completion.term)
                << "\",\"documents\":" << completion.document_frequency << "}";
            first = false;
        }
    }
    out << "]}\n";
    return HttpResponse{200, "application/json", out.str()};
}

std::string SearchService::title_of(const std::string& document_id) const {
    if (corpus_ == nullptr) {
        return document_id;
    }
    const std::optional<Document> doc = corpus_->read(document_id);
    if (!doc.has_value() || doc->title.empty()) {
        return document_id;
    }
    return doc->title;
}

std::string SearchService::url_of(const std::string& document_id) const {
    const auto found = urls_.find(document_id);
    return found == urls_.end() ? std::string() : found->second;
}

SearchService::Answer SearchService::answer(const HttpRequest& request) {
    Answer out;
    out.options = defaults_;

    const auto q = request.query.find("q");
    out.query = q == request.query.end() ? std::string() : q->second;

    const auto limit = request.query.find("limit");
    if (limit != request.query.end() && !parse_limit(limit->second, out.options.limit)) {
        out.error = "limit must be a whole number from 1 to 100";
        return out;
    }

    const auto weight = request.query.find("pagerank");
    if (weight != request.query.end() &&
        !parse_weight(weight->second, out.options.pagerank_weight)) {
        out.error = "pagerank must be a non-negative number";
        return out;
    }

    const auto graph = request.query.find("graph");
    if (graph != request.query.end()) {
        const std::optional<LinkGraph> parsed = parse_link_graph(graph->second);
        if (!parsed.has_value()) {
            out.error = "graph must be page, site or home";
            return out;
        }
        out.options.pagerank_graph = *parsed;
    }

    const auto max_words = request.query.find("max_words");
    if (max_words != request.query.end() &&
        !parse_word_count(max_words->second, out.options.pagerank_max_words)) {
        out.error = "max_words must be a whole number below 10000";
        return out;
    }

    const PageRankPrior* prior = priors_.find(out.options.pagerank_graph);
    if (prior == nullptr && out.options.pagerank_weight > 0.0) {
        out.error = std::string("no ") + link_graph_name(out.options.pagerank_graph) +
                    " ranking for this index";
        return out;
    }

    if (out.query.find_first_not_of(" \t") == std::string::npos) {
        return out;
    }

    std::string error;
    std::optional<std::vector<QueryResult>> results =
        run_query(index_, corpus_, out.query, out.options, error, prior);
    if (!results.has_value()) {
        out.error = error;
        return out;
    }
    out.results = std::move(*results);
    out.suggestion = correct_query(out.query, index_, tree());
    return out;
}

HttpResponse SearchService::html(const Answer& answer) const {
    std::ostringstream content;
    if (!answer.error.empty()) {
        content << "<p class=\"note\">" << html_escape(answer.error) << "</p>";
    }

    if (answer.suggestion.has_value()) {
        content << "<p>Did you mean <a href=\"/search?q=" << url_encode(*answer.suggestion)
                << "\">" << html_escape(*answer.suggestion) << "</a>?</p>";
    }

    const bool asked = answer.query.find_first_not_of(" \t") != std::string::npos;
    if (asked && answer.error.empty() && answer.results.empty()) {
        content << "<p class=\"note\">No documents match.</p>";
    }

    if (!answer.results.empty()) {
        content << "<ol>";
        for (const QueryResult& result : answer.results) {
            const std::string url = url_of(result.document_id);
            const std::string title = html_escape(title_of(result.document_id));
            content << "<li>";
            if (url.empty()) {
                content << "<strong>" << title << "</strong>";
            } else {
                content << "<a href=\"" << html_escape(url) << "\">" << title << "</a>"
                        << "<div class=\"url\">" << html_escape(url) << "</div>";
            }
            content << "<div class=\"score\">" << html_escape(result.document_id) << " &middot; "
                    << std::fixed << std::setprecision(4) << result.score << "</div>";
            if (!result.snippet.empty()) {
                content << "<p class=\"snippet\">" << html_escape(flatten(result.snippet))
                        << "</p>";
            }
            content << "</li>";
        }
        content << "</ol>";
    }

    HttpResponse response;
    response.status = answer.error.empty() ? 200 : 400;
    response.body = page(answer.query, content.str());
    return response;
}

HttpResponse SearchService::json(const Answer& answer) const {
    std::ostringstream out;
    out << "{\"query\":\"" << json_escape(answer.query) << "\"";
    if (!answer.error.empty()) {
        out << ",\"error\":\"" << json_escape(answer.error) << "\"}";
    } else {
        out << ",\"suggestion\":";
        if (answer.suggestion.has_value()) {
            out << "\"" << json_escape(*answer.suggestion) << "\"";
        } else {
            out << "null";
        }
        out << ",\"results\":[";
        for (std::size_t i = 0; i < answer.results.size(); i++) {
            const QueryResult& result = answer.results[i];
            out << (i == 0 ? "" : ",") << "{\"id\":\"" << json_escape(result.document_id)
                << "\",\"score\":" << std::fixed << std::setprecision(4) << result.score
                << ",\"title\":\"" << json_escape(title_of(result.document_id))
                << "\",\"url\":\"" << json_escape(url_of(result.document_id))
                << "\",\"snippet\":\"" << json_escape(flatten(result.snippet)) << "\"}";
        }
        out << "]}";
    }
    out << "\n";

    HttpResponse response;
    response.status = answer.error.empty() ? 200 : 400;
    response.content_type = "application/json";
    response.body = out.str();
    return response;
}

HttpResponse SearchService::handle(const HttpRequest& request) {
    if (request.method != "GET") {
        return HttpResponse{405, "text/plain; charset=utf-8", "only GET is supported\n"};
    }
    if (request.path == "/" || request.path == "/search") {
        return html(answer(request));
    }
    if (request.path == "/api/search") {
        return json(answer(request));
    }
    if (request.path == "/api/complete") {
        return completions(request);
    }
    return HttpResponse{404, "text/plain; charset=utf-8", "not found\n"};
}

std::unordered_map<std::string, std::string> read_urls(const std::string& path) {
    std::unordered_map<std::string, std::string> urls;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t tab = line.find('\t');
        if (tab != std::string::npos) {
            urls.emplace(line.substr(0, tab), line.substr(tab + 1));
        }
    }
    return urls;
}

bool serve(SearchService& service, int port, std::string& error) {
    const int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        error = std::string("socket: ") + std::strerror(errno);
        return false;
    }

    const int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<std::uint16_t>(port));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(listener, reinterpret_cast<const sockaddr*>(&address), sizeof address) != 0 ||
        listen(listener, 16) != 0) {
        error = "cannot listen on port " + std::to_string(port) + ": " + std::strerror(errno);
        close(listener);
        return false;
    }

    std::cout << "listening on http://127.0.0.1:" << port << "/" << std::endl;

    while (true) {
        const int client = accept(listener, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            error = std::string("accept: ") + std::strerror(errno);
            close(listener);
            return false;
        }

        timeval timeout{5, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        std::string raw;
        char buffer[4096];
        while (raw.find("\r\n\r\n") == std::string::npos && raw.size() < 16384) {
            const ssize_t got = recv(client, buffer, sizeof buffer, 0);
            if (got <= 0) {
                break;
            }
            raw.append(buffer, static_cast<std::size_t>(got));
        }

        const std::optional<HttpRequest> request = parse_request(raw);
        const std::string reply = serialize(
            request.has_value() ? service.handle(*request)
                                : HttpResponse{400, "text/plain; charset=utf-8", "bad request\n"});

        std::size_t sent = 0;
        while (sent < reply.size()) {
            const ssize_t wrote = send(client, reply.data() + sent, reply.size() - sent, 0);
            if (wrote <= 0) {
                break;
            }
            sent += static_cast<std::size_t>(wrote);
        }
        close(client);
    }
}
