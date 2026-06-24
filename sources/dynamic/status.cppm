module;

#include <string>
#include <utility>

export module weqeqq.terminal:status;

import :protocol;
import :spinner;
import :text;
import :style;
import :live;
import :console;

export namespace weqeqq::terminal {

class Status {
 public:
  Status(Console& console, const std::string& message_markup,
         const std::string& spinner_name = "dots", Style style = {})
      : spinner_(spinner_name, Text::FromMarkup(message_markup), style),
        live_(console, Renderable(spinner_)) {}

  void Update(const std::string& message_markup) {
    spinner_.text = Text::FromMarkup(message_markup);
    live_.Update(Renderable(spinner_));
  }

  void Start() { live_.Start(); }
  void Stop() { live_.Stop(); }
  void Refresh() { live_.Update(Renderable(spinner_)); }

 private:
  Spinner spinner_;
  Live live_;
};

}
