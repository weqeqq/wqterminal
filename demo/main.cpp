#include <chrono>
#include <cmath>
#include <print>
#include <string>
#include <thread>
#include <utility>
#include <vector>

import weqeqq.terminal;

namespace t = weqeqq::terminal;

static void SleepMs(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
  t::Console console;
  console.color_system = t::ColorSystem::kTrueColor;
  console.width = 64;

  console.Print(
      t::Renderable(t::Rule(t::Text::FromMarkup("[bold]weqeqq.terminal[/]"))));
  console.Print(
      "[bold red]bold red[/]  [green italic]green italic[/]  "
      "[underline]underline[/]  [reverse] reverse [/]");
  console.Print(
      "[#ff8800 on #202020] truecolor on dark [/] "
      "[link=https://example.com]a hyperlink[/link]");
  console.Print();

  console.Print(":rocket: launch  :sparkles: shiny  :heavy_check_mark: done");
  console.Print(
      "auto-highlight: 42 items, 3.14 pi, true, https://rich.dev, [[1, 2, 3]]");
  console.Print(
      "themed names: [repr.number]123[/] [repr.str]'hi'[/] "
      "[repr.bool_true]done[/]");
  console.Print(t::Renderable(t::Text::FromAnsi(
      "from_ansi: \x1b[1;32mgreen bold\x1b[0m -> back to normal")));
  console.Print();

  t::Bar gauge(100, 20, 70);
  gauge.width = 50;
  console.Print(t::Renderable(gauge));
  console.Print("sizes: " + t::DecimalSize(1536000) + "  /  " +
                t::BinarySize(1536000));
  console.Print();

  console.Print("[bold]Gradient (Blend):[/]");
  {
    t::Color ca = t::Color::Parse("#ff3366");
    t::Color cb = t::Color::Parse("#33ccff");
    t::Text grad;
    const int n = 60;
    for (int i = 0; i < n; ++i) {
      t::Style st;
      st.color = ca.Blend(cb, static_cast<double>(i) / (n - 1));
      grad.Append("━", st);
    }
    console.Print(t::Renderable(grad));
  }
  console.Print();

  t::Table table;
  table.box = t::kRounded;
  table.AddColumn("[bold]Lang[/]");
  table.AddColumn("[bold]Year[/]", t::JustifyMethod::kRight);
  table.AddColumn("[bold]Notes[/]");
  table.AddRowText({"C++", "1985", "[green]Systems, performance[/]"});
  table.AddRowText({"Python", "1991", "[cyan]Readable, batteries included[/]"});
  table.AddRowText({"Rust", "2010", "[magenta]Memory safety without GC[/]"});
  console.Print(t::Renderable(table));
  console.Print();

  console.Print("[bold]Box styles:[/]");
  {
    std::vector<std::pair<const char*, const t::Box*>> kinds = {
        {"rounded", &t::kRounded}, {"heavy", &t::kHeavy},
        {"double", &t::kDouble},   {"ascii", &t::kAscii},
        {"minimal", &t::kMinimal},
    };
    std::vector<t::Renderable> panels;
    for (auto& [name, box] : kinds) {
      t::Panel p(t::Renderable(t::Text(name)), *box);
      p.expand = false;
      panels.emplace_back(p);
    }
    console.Print(t::Renderable(t::Columns(std::move(panels), 1)));
  }
  console.Print();

  t::Panel panel(t::Renderable(t::Text::FromMarkup(
      "This is a [bold]Panel[/]. It wraps any renderable in a border and can "
      "carry a title. The body text wraps to the available width.")));
  panel.WithTitle("[bold yellow] Info [/]");
  panel.WithBorderStyle(t::Style::Parse("cyan"));
  console.Print(t::Renderable(panel));
  console.Print();

  t::Tree tree(t::Renderable(t::Text::FromMarkup("[bold]project[/]")));
  auto& sources = tree.Add(t::Renderable(t::Text("sources")));
  sources.Add(t::Renderable(t::Text("core")));
  sources.Add(t::Renderable(t::Text("widgets")));
  sources.Add(t::Renderable(t::Text("dynamic")));
  tree.Add(t::Renderable(t::Text("demo")));
  tree.Add(t::Renderable(t::Text("tests")));
  console.Print(t::Renderable(tree));
  console.Print();

  console.Print("[bold]Layout:[/]");
  {
    t::Layout root;
    t::Layout top(t::Renderable(t::Panel(
                      t::Renderable(t::Text::FromMarkup("[bold]header[/]")))),
                  "top");
    top.size = 3;
    t::Layout bottom("bottom");
    root.SplitColumn({top, bottom});

    t::Layout left(t::Renderable(t::Panel(t::Renderable(t::Text("left pane")))),
                   "left");
    t::Layout right(
        t::Renderable(t::Panel(t::Renderable(t::Text("right pane")))), "right");
    root["bottom"].SplitRow({left, right});

    auto o = console.Options();
    o.height = 7;
    console.Print(t::Renderable(root), o);
  }
  console.Print();

  console.Print("[bold]Live display:[/] (redraws in place)");
  {
    t::Live live(console, t::Renderable(t::Text("")));
    live.Start();
    for (int frame = 0; frame <= 28; ++frame) {
      t::Table grid = t::Table::Grid();
      grid.pad_right = 1;
      grid.AddColumn("", t::JustifyMethod::kRight, {}, 5);
      grid.AddColumn("");
      grid.AddColumn("", t::JustifyMethod::kRight, {}, 5);

      auto add_gauge = [&](const char* label, double pct, const char* color) {
        pct = std::clamp(pct, 0.0, 100.0);
        t::ProgressBar b(100, pct);
        b.width = 30;
        b.complete_style = t::Style::Parse(color);
        grid.AddRow({
            t::Renderable(t::Text(label)),
            t::Renderable(b),
            t::Renderable(t::Text(std::to_string(static_cast<int>(pct)) + "%")),
        });
      };
      add_gauge("cpu", 50 + 45 * std::sin(frame * 0.40), "magenta");
      add_gauge("mem", frame * 4.0, "cyan");
      add_gauge("net", 35 + 35 * std::sin(frame * 0.70 + 1.0), "yellow");

      t::Panel dash{t::Renderable(grid)};
      dash.WithTitle("[bold] system monitor [/]");
      dash.WithBorderStyle(t::Style::Parse("green"));
      dash.expand = false;
      live.Update(t::Renderable(dash));
      SleepMs(70);
    }
    live.Stop();
  }
  console.Print("[green]✓ live display finished[/]");
  console.Print();

  console.Print("[bold]Progress:[/]");
  {
    t::Progress progress(console);
    progress.columns = {t::ProgressColumn::kDescription,
                        t::ProgressColumn::kBar, t::ProgressColumn::kPercentage,
                        t::ProgressColumn::kMofN,
                        t::ProgressColumn::kRemaining};
    int dl = progress.AddTask("Download", 100);
    int proc = progress.AddTask("Process", 100);
    progress.Start();
    for (int i = 0; i <= 100; i += 4) {
      progress.Update(dl, static_cast<double>(i));
      progress.Update(proc, i * 0.75);
      SleepMs(25);
    }
    progress.Stop();
  }
  console.Print("[green]✓ all tasks complete[/]");
  console.Print();

  {
    t::Status status(console, "[cyan]Finalizing...[/]");
    status.Start();
    for (int i = 0; i < 16; ++i) {
      status.Refresh();
      SleepMs(70);
    }
    status.Stop();
  }
  console.Print("[bold green]Done.[/]");

  if (console.is_terminal) {
    std::string name = t::Prompt::Ask(console, "[bold]Your name[/]", "anon");
    console.Print("Hello, [bold cyan]" + name + "[/]! :wave:");
  }
  return 0;
}
