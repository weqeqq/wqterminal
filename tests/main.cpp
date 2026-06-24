#include <cstddef>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

import weqeqq.terminal;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace t = weqeqq::terminal;

std::vector<std::string> SplitToLines(const std::string& text) {
  std::vector<std::string> lines;
  std::string cur;
  for (char c : text) {
    if (c == '\n') {
      lines.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  return lines;
}

std::string PlainOf(const t::Line& line) {
  std::string s;
  for (const auto& seg : line) s += seg.text;
  return s;
}

const Suite kCells("cells", [] {
  Test("ascii length", [] { Expect(t::CellLen("hello"), Eq(5)); });
  Test("wide chars count double", [] {
    Expect(t::CellLen("\xe4\xbd\xa0\xe5\xa5\xbd"), Eq(4));
  });
  Test("combining mark is zero width", [] {
    Expect(t::CellLen("a\xcc\x80"), Eq(1));
  });
  Test("set cell size truncates", [] {
    Expect(t::SetCellSize("hello", 3), Eq("hel"));
  });
  Test("set cell size pads", [] {
    Expect(t::SetCellSize("hi", 4), Eq("hi  "));
  });
});

const Suite kColor("color", [] {
  Test("parse standard name", [] {
    t::Color red = t::Color::Parse("red");
    Expect(red.type == t::ColorType::kStandard);
    Expect(red.number, Optional(1));
  });
  Test("parse hex truecolor", [] {
    t::Color hex = t::Color::Parse("#ff0000");
    Expect(hex.type == t::ColorType::kTrueColor);
    Require(hex.triplet.has_value());
    Expect(hex.triplet->red == 255 && hex.triplet->green == 0 &&
           hex.triplet->blue == 0);
  });
  Test("downgrade to eight bit", [] {
    t::Color hex8 =
        t::Color::Parse("#ff0000").Downgrade(t::ColorSystem::kEightBit);
    Expect(hex8.type == t::ColorType::kEightBit);
  });
  Test("downgrade to standard lands on red", [] {
    t::Color hexstd =
        t::Color::Parse("#ff0000").Downgrade(t::ColorSystem::kStandard);
    Expect(hexstd.type == t::ColorType::kStandard);
    Require(hexstd.number.has_value());
    Expect(hexstd.number.value() == 9 || hexstd.number.value() == 1);
  });
  Test("parse color() index", [] {
    t::Color c196 = t::Color::Parse("color(196)");
    Expect(c196.type == t::ColorType::kEightBit);
    Expect(c196.number, Optional(196));
  });
  Test("ansi codes", [] {
    Expect(t::Color::Parse("red").GetAnsiCodes(true)[0], Eq("31"));
    Expect(t::Color::Parse("bright_red").GetAnsiCodes(true)[0], Eq("91"));
  });
});

const Suite kStyle("style", [] {
  Test("parse attributes and colors", [] {
    t::Style s = t::Style::Parse("bold red on white");
    Expect(s.Get(t::kBold), Optional(true));
    Require(s.color.has_value());
    Expect(s.color->number, Optional(1));
    Require(s.bgcolor.has_value());
    Expect(s.bgcolor->number, Optional(7));
  });
  Test("render emits sgr", [] {
    t::Style s = t::Style::Parse("bold red on white");
    std::string rendered = s.Render("X", t::ColorSystem::kTrueColor);
    Expect(rendered.find("\x1b[") != std::string::npos);
    Expect(rendered.find("1;31;47") != std::string::npos);
    Expect(rendered.find("X\x1b[0m") != std::string::npos);
  });
  Test("not bold turns attribute off", [] {
    t::Style nb = t::Style::Parse("not bold");
    Expect(nb.Get(t::kBold), Optional(false));
  });
  Test("combine: other wins, inherits otherwise", [] {
    t::Style combined = t::Style::Parse("red") + t::Style::Parse("bold blue");
    Require(combined.color.has_value());
    Expect(combined.color->number, Optional(4));
    Expect(combined.Get(t::kBold), Optional(true));
  });
});

const Suite kSegment("segment", [] {
  Test("split lines on newline", [] {
    std::vector<t::Segment> segs = {t::Segment("ab"), t::Segment("c\nd"),
                                    t::Segment("e")};
    auto lines = t::SplitLines(segs);
    Require(lines.size() == std::size_t{2});
    Expect(t::LineLength(lines[0]), Eq(3));
    Expect(t::LineLength(lines[1]), Eq(2));
  });
  Test("adjust line length pads", [] {
    std::vector<t::Segment> segs = {t::Segment("ab"), t::Segment("c\nd"),
                                    t::Segment("e")};
    auto lines = t::SplitLines(segs);
    auto padded = t::AdjustLineLength(lines[1], 5);
    Expect(t::LineLength(padded), Eq(5));
  });
});

const Suite kMarkup("markup", [] {
  Test("tag spans the styled run", [] {
    t::Text m = t::Text::FromMarkup("[bold red]Hi[/] there");
    Expect(m.Plain(), Eq("Hi there"));
    Require(m.Spans().size() == std::size_t{1});
    Expect(m.Spans()[0].start == 0 && m.Spans()[0].end == 2);
    Expect(m.Spans()[0].style.Get(t::kBold), Optional(true));
  });
  Test("escaped bracket is literal", [] {
    Expect(t::Text::FromMarkup("a \\[b] c").Plain(), Eq("a [b] c"));
  });
  Test("non-tag bracket stays literal", [] {
    Expect(t::Text::FromMarkup("[123] x").Plain(), Eq("[123] x"));
  });
});

const Suite kWrapping("wrapping", [] {
  Test("wrap fits within width", [] {
    t::Text para("aaa bbb ccc ddd");
    auto wlines = para.Wrap(7, t::JustifyMethod::kLeft,
                            t::OverflowMethod::kFold, false, 8);
    Expect(wlines.size() >= std::size_t{2});
    for (auto& wl : wlines) Expect(wl.CellLength(), Le(7));
  });
  Test("long word folds", [] {
    t::Text longw("abcdefghij");
    auto flines = longw.Wrap(4, t::JustifyMethod::kDefault,
                             t::OverflowMethod::kFold, false, 8);
    Expect(flines.size(), Eq(std::size_t{3}));
  });
});

const Suite kConsole("console", [] {
  Test("render markup to string", [] {
    t::Console con;
    con.color_system = t::ColorSystem::kTrueColor;
    std::string out =
        con.RenderToString(t::Renderable(t::Text::FromMarkup("[bold]X[/]")));
    Expect(out.find("\x1b[1mX\x1b[0m") != std::string::npos);
  });
});

const Suite kWidgets("widgets", [] {
  Test("table renders well-formed grid", [] {
    t::Console con;
    con.no_color = true;
    con.width = 40;
    auto opts40 = con.Options();
    opts40.max_width = 40;

    t::Table tbl;
    tbl.box = t::kSquare;
    tbl.AddColumn("A");
    tbl.AddColumn("B");
    tbl.AddRowText({"1", "2"});
    std::string tout =
        con.SegmentsToString(con.Render(t::Renderable(tbl), opts40));

    auto tlines = SplitToLines(tout);
    Expect(tlines.size(), Eq(std::size_t{5}));
    int w0 = t::CellLen(tlines[0]);
    bool all_eq = true;
    for (auto& l : tlines)
      if (t::CellLen(l) != w0) all_eq = false;
    Expect(all_eq);
  });
  Test("panel expands to full width", [] {
    t::Console con;
    con.no_color = true;
    con.width = 40;
    auto opts40 = con.Options();
    opts40.max_width = 40;

    t::Panel pnl{t::Renderable(t::Text("hello"))};
    pnl.expand = true;
    std::string pout =
        con.SegmentsToString(con.Render(t::Renderable(pnl), opts40));
    std::string first = pout.substr(0, pout.find('\n'));
    Expect(t::CellLen(first), Eq(40));
  });
});

const Suite kProgress("progress", [] {
  Test("spinner renders text", [] {
    t::Spinner sp("line");
    Expect(sp.RenderText().IsEmpty(), IsFalse());
  });
  Test("progress bar emits the bar glyph", [] {
    t::Console con;
    con.width = 40;
    auto opts40 = con.Options();
    opts40.max_width = 40;

    t::ProgressBar bar(100, 50);
    bar.width = 10;
    std::string bout =
        con.SegmentsToString(con.Render(t::Renderable(bar), opts40));
    Expect(bout.find("━") != std::string::npos);
  });
  Test("live writes control codes via capture", [] {
    t::Console lc;
    lc.color_system = t::ColorSystem::kStandard;
    lc.BeginCapture();
    t::Progress prog(lc);
    int task = prog.AddTask("work", 100);
    prog.Start();
    prog.Update(task, 50.0);
    prog.Update(task, 100.0);
    prog.Stop();
    std::string cap = lc.EndCapture();
    Expect(cap.find("\x1b[?25l") != std::string::npos);
    Expect(cap.find("\x1b[?25h") != std::string::npos);
    Expect(cap.find("\x1b[2A") != std::string::npos ||
           cap.find("\x1b[1A") != std::string::npos);
    Expect(prog.Finished(), IsTrue());
  });
});

const Suite kTui("tui", [] {
  Test("style meta survives combine and affects equality", [] {
    t::Style m1;
    m1.meta["@click"] = "app.bell";
    t::Style m2 = t::Style::Parse("bold") + m1;
    Require(m2.meta.count("@click") != 0);
    Expect(m2.meta.at("@click"), Eq("app.bell"));
    Expect(m2.Get(t::kBold), Optional(true));
    Expect(!(t::Style::Parse("bold") == m2));
  });
  Test("markup @click attaches meta", [] {
    t::Text mt = t::Text::FromMarkup("[@click=do.thing]x[/]");
    Require(mt.Spans().size() == std::size_t{1});
    Require(mt.Spans()[0].style.meta.count("@click") != 0);
    Expect(mt.Spans()[0].style.meta.at("@click"), Eq("do.thing"));
  });
  Test("segment crop and divide", [] {
    t::Line cl = {t::Segment("abcdef")};
    Expect(PlainOf(t::CropLine(cl, 2, 3)), Eq("cde"));
    auto parts = t::Divide(cl, {2, 4});
    Require(parts.size() == std::size_t{3});
    Expect(PlainOf(parts[0]), Eq("ab"));
    Expect(PlainOf(parts[1]), Eq("cd"));
    Expect(PlainOf(parts[2]), Eq("ef"));
  });
  Test("cropping a wide glyph at a boundary yields a space", [] {
    t::Line wl = {t::Segment("\xe4\xbd\xa0")};
    Expect(PlainOf(t::CropLine(wl, 0, 1)), Eq(" "));
  });
  Test("control sequences", [] {
    Expect(t::control::MoveTo(0, 0), Eq("\x1b[1;1H"));
    Expect(t::control::MoveTo(4, 2), Eq("\x1b[3;5H"));
    Expect(t::control::EnableAltScreen(), Eq("\x1b[?1049h"));
  });
  Test("console alt-screen toggle", [] {
    t::Console ac;
    ac.BeginCapture();
    ac.SetAltScreen(true);
    Expect(ac.in_alt_screen(), IsTrue());
    ac.SetAltScreen(false);
    std::string cap = ac.EndCapture();
    Expect(cap.find("\x1b[?1049h") != std::string::npos);
    Expect(cap.find("\x1b[?1049l") != std::string::npos);
  });
  Test("theme styles and markup resolution", [] {
    t::Console tc;
    Expect(tc.GetStyle("table.header").Get(t::kBold), Optional(true));
    t::Text rn = tc.RenderStr("[repr.number]42[/]");
    Require(rn.Spans().size() == std::size_t{1});
    Expect(rn.Spans()[0].style.color.has_value(), IsTrue());
  });
});

const Suite kColorNames("color-names", [] {
  Test("named eight-bit colors", [] {
    t::Color dsb = t::Color::Parse("deep_sky_blue1");
    Expect(dsb.type == t::ColorType::kEightBit);
    Expect(dsb.number, Optional(39));
    t::Color g37 = t::Color::Parse("grey37");
    Expect(g37.type == t::ColorType::kEightBit);
    Expect(g37.number, Optional(59));
  });
  Test("blend red to blue at midpoint", [] {
    t::Color mid =
        t::Color::Parse("#ff0000").Blend(t::Color::Parse("#0000ff"), 0.5);
    Require(mid.triplet.has_value());
    Expect(mid.triplet->red == 128 && mid.triplet->green == 0 &&
           mid.triplet->blue == 128);
  });
  Test("box substitute swaps unicode for ascii", [] {
    Expect(t::kRounded.Substitute(true).top_left, Eq("+"));
    Expect(t::kRounded.Substitute(false).top_left, Eq("╭"));
  });
});

const Suite kAnsi("ansi", [] {
  Test("from ansi parses sgr", [] {
    t::Text fa = t::Text::FromAnsi("\x1b[1;31mhi\x1b[0m!");
    Expect(fa.Plain(), Eq("hi!"));
    Expect(fa.GetStyleAtOffset(0).Get(t::kBold), Optional(true));
    Expect(fa.GetStyleAtOffset(0).color.has_value(), IsTrue());
    Expect(fa.GetStyleAtOffset(2).color.has_value(), IsFalse());
  });
  Test("replace emoji", [] {
    Expect(t::ReplaceEmoji(":rocket: x"), Eq("\xf0\x9f\x9a\x80 x"));
    Expect(t::Text::FromMarkup(":rocket:").Plain(), Eq("\xf0\x9f\x9a\x80"));
  });
  Test("highlight regex", [] {
    t::Text hr("id 42 ok");
    hr.HighlightRegex("[0-9]+", t::Style::Parse("bold"));
    Expect(hr.GetStyleAtOffset(3).Get(t::kBold), Optional(true));
    Expect(static_cast<bool>(hr.GetStyleAtOffset(0)), IsFalse());
  });
  Test("assemble styled spans", [] {
    t::Text asmbl =
        t::Text::Assemble({{"a", t::Style::Parse("red")}, {"b", t::Style{}}});
    Expect(asmbl.Plain(), Eq("ab"));
    Expect(asmbl.GetStyleAtOffset(0).color.has_value(), IsTrue());
  });
});

const Suite kFilesize("filesize", [] {
  Test("decimal sizes", [] {
    Expect(t::DecimalSize(500), Eq("500 bytes"));
    Expect(t::DecimalSize(1500), Eq("1.5 kB"));
  });
  Test("binary sizes", [] { Expect(t::BinarySize(1536), Eq("1.5 KiB")); });
  Test("bar widget emits glyph", [] {
    t::Console con;
    con.no_color = true;
    con.width = 40;
    auto opts40 = con.Options();
    opts40.max_width = 40;

    t::Bar bb(100, 0, 50);
    bb.width = 10;
    std::string bbo =
        con.SegmentsToString(con.Render(t::Renderable(bb), opts40));
    Expect(bbo.find("━") != std::string::npos);
  });
  Test("grid renders space-separated without box", [] {
    t::Console con;
    con.no_color = true;
    con.width = 40;
    auto opts40 = con.Options();
    opts40.max_width = 40;

    t::Table g = t::Table::Grid();
    g.AddRow({t::Renderable(t::Text("a")), t::Renderable(t::Text("b"))});
    std::string gout =
        con.SegmentsToString(con.Render(t::Renderable(g), opts40));
    Expect(gout.find("a b") != std::string::npos);
    Expect(gout.find("─") == std::string::npos);
  });
  Test("table renders title", [] {
    t::Console con;
    con.no_color = true;
    con.width = 40;
    auto opts40 = con.Options();
    opts40.max_width = 40;

    t::Table tt;
    tt.box = t::kSquare;
    tt.AddColumn("X");
    tt.AddRowText({"1"});
    tt.title = t::Text("Tbl");
    std::string tto =
        con.SegmentsToString(con.Render(t::Renderable(tt), opts40));
    Expect(tto.find("Tbl") != std::string::npos);
  });
  Test("constrain limits measured width", [] {
    t::Console con;
    con.no_color = true;
    con.width = 40;
    auto cm = con.Measure(
        t::Renderable(t::Constrain(t::Renderable(t::Text("hello world")), 5)));
    Expect(cm.maximum, Le(5));
  });
});

const Suite kSpinnerRegistry("spinner-registry", [] {
  Test("known spinner has frames", [] {
    Expect(t::Spinner("clock").frames.empty(), IsFalse());
  });
  Test("unknown spinner falls back to dots", [] {
    Expect(t::Spinner("does_not_exist").frames.empty(), IsFalse());
  });
});

const Suite kProgressColumns("progress-columns", [] {
  Test("percentage and bar columns render", [] {
    t::Console pc;
    pc.color_system = t::ColorSystem::kStandard;
    pc.BeginCapture();
    t::Progress pr(pc);
    int id = pr.AddTask("dl", 100);
    pr.Start();
    pr.Update(id, 50.0);
    pr.Stop();
    std::string cap = pc.EndCapture();
    Expect(cap.find("50%") != std::string::npos);
    Expect(cap.find("━") != std::string::npos);
  });
});

const Suite kPrompt("prompt", [] {
  Test("confirm accepts yes", [] {
    std::istringstream in("y\n");
    std::streambuf* old = std::cin.rdbuf(in.rdbuf());
    t::Console pc2;
    pc2.no_color = true;
    pc2.BeginCapture();
    bool yes = t::Confirm::Ask(pc2, "ok?");
    (void)pc2.EndCapture();
    std::cin.rdbuf(old);
    Expect(yes, IsTrue());
  });
  Test("int prompt rejects trailing garbage and reprompts", [] {
    std::istringstream in("12abc\n7\n");
    std::streambuf* old = std::cin.rdbuf(in.rdbuf());
    t::Console pc;
    pc.no_color = true;
    pc.BeginCapture();
    long long v = t::IntPrompt::Ask(pc, "n?");
    (void)pc.EndCapture();
    std::cin.rdbuf(old);
    Expect(v, Eq(7LL));
  });
});

const Suite kLayout("layout", [] {
  Test("split regions fill the width", [] {
    t::Console lc;
    lc.no_color = true;

    t::Layout header(t::Renderable(t::Text("HEADER")), "header");
    header.size = 1;
    t::Layout body("body");
    t::Layout root;
    root.SplitColumn({header, body});

    t::Layout side(t::Renderable(t::Text("side")), "side");
    side.size = 10;
    t::Layout main(t::Renderable(t::Text("main")), "main");
    root["body"].SplitRow({side, main});

    auto o = lc.Options();
    o.max_width = 30;
    o.height = 5;
    std::string out = lc.SegmentsToString(lc.Render(t::Renderable(root), o));

    auto ll = SplitToLines(out);
    Require(ll.size() >= std::size_t{5});
    bool all30 = true;
    for (int i = 0; i < 5; ++i)
      if (t::CellLen(ll[i]) != 30) all30 = false;
    Expect(all30);
    Expect(out.find("HEADER") != std::string::npos);
    Expect(out.find("side") != std::string::npos);
    Expect(out.find("main") != std::string::npos);
    Expect(root.Find("main"), NotNull());
    Expect(root.Find("nope"), IsNull());
  });
});

const Suite kRegressions("regressions", [] {
  Test("from ansi skips non-sgr csi", [] {
    t::Text fc = t::Text::FromAnsi("\x1b[2J\x1b[1mhi\x1b[0m");
    Expect(fc.Plain(), Eq("hi"));
    Expect(fc.GetStyleAtOffset(0).Get(t::kBold), Optional(true));
  });
  Test("truncate to width 0 yields empty", [] {
    t::Text tz("hello");
    tz.Truncate(0, t::OverflowMethod::kEllipsis, false);
    Expect(tz.Length(), Eq(0));
  });
  Test("invalid regex is ignored, not thrown", [] {
    t::Text hg("x");
    hg.HighlightRegex("(", t::Style::Parse("bold"));
    Expect(hg.Plain(), Eq("x"));
  });
  Test("bad color() throws ColorParseError", [] {
    Expect([] { t::Color::Parse("color()"); },
           Throws<t::ColorParseError>());
  });
  Test("progress bar with total<=0 renders empty back track", [] {
    t::Console cc;
    cc.color_system = t::ColorSystem::kEightBit;
    cc.no_color = false;
    auto co = cc.Options();
    co.max_width = 8;
    t::ProgressBar zb(0, 0);
    zb.width = 8;
    std::string z = cc.SegmentsToString(cc.Render(t::Renderable(zb), co));
    Expect(z.find("237") != std::string::npos);
  });
});
}
