// systemc_sim/sc_main.cpp
#include <systemc.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <array>

extern "C" {
#include "../MusicProcessor/kissfft/kiss_fftr.h"
}

using namespace sc_core;

// ===================== Types =====================

struct SampleBlock {
  std::vector<float> s; // float samples [-1..1]
  sc_time ts;
};

struct AudioWindow {
  std::vector<float> x; // fft_size samples
  sc_time ts;
};

struct Spectrum {
  std::vector<float> mag; // fft_size/2+1 magnitudes
  float max_mag;          // max over selected bins (like display_spectrum)
  std::array<float, 40> bars; // normalized bars [0..1] for bins 1..40
  sc_time ts;
};

// sc_fifo requires operator<< for dump()
static inline std::ostream& operator<<(std::ostream& os, const SampleBlock& b) {
  os << "SampleBlock{n=" << b.s.size() << ", ts=" << b.ts << "}";
  return os;
}
static inline std::ostream& operator<<(std::ostream& os, const AudioWindow& w) {
  os << "AudioWindow{n=" << w.x.size() << ", ts=" << w.ts << "}";
  return os;
}
static inline std::ostream& operator<<(std::ostream& os, const Spectrum& sp) {
  os << "Spectrum{n=" << sp.mag.size() << ", max_mag=" << sp.max_mag << ", ts=" << sp.ts << "}";
  return os;
}

// ===================== Audio Source (mô phỏng input) =====================
// Thay cho FFmpeg+ALSA: tạo ra samples float giống output convert_samples_to_float().
// Có kick định kỳ để spectrum/bars thay đổi rõ rệt.
SC_MODULE(AudioSourceSC) {
  sc_fifo_out<SampleBlock> out;

  int sample_rate = 48000;
  int block_size  = 512;                 // giống kiểu block bạn feed vào ring_buffer
  sc_time block_period = sc_time(10, SC_MS);

  SC_CTOR(AudioSourceSC) { SC_THREAD(run); }

  void run() {
    constexpr double PI = 3.14159265358979323846;

    double t = 0.0;
    const double dt = 1.0 / sample_rate;

    // Compose a signal with:
    // - bass-ish kick (low freq burst)
    // - mid sine
    // => spectrum nhìn rõ hơn
    while (true) {
      SampleBlock b;
      b.s.resize(block_size);
      b.ts = sc_time_stamp();

      for (int i = 0; i < block_size; i++) {
        // Mid tone
        float mid = 0.08f * std::sin(2.0 * PI * 220.0 * t);

        // A little high tone
        float hi  = 0.03f * std::sin(2.0 * PI * 1200.0 * t);

        // Kick burst every 0.5s: use decaying low-frequency sine burst
        float kick = 0.0f;
        double phase = std::fmod(t, 0.5);
        if (phase < 0.06) { // 60ms burst
          float env = float(1.0 - phase / 0.06);              // linear decay
          float low = std::sin(2.0 * PI * 60.0 * t);          // 60Hz-ish
          kick = 0.9f * env * low;
        }

        float x = mid + hi + kick;

        // Clamp like your convert_samples_to_float()
        if (x > 1.0f) x = 1.0f;
        if (x < -1.0f) x = -1.0f;

        b.s[i] = x;
        t += dt;
      }

      out.write(b);
      wait(block_period);
    }
  }
};

// ===================== Ring Buffer model (write + read_all) =====================
// Like ring_buffer_write() then ring_buffer_read_all() into input_buffer.
SC_MODULE(RingBufferSC) {
  sc_fifo_in<SampleBlock>  in;
  sc_fifo_out<AudioWindow> out;

  int fft_size = 1024;
  sc_time latency = sc_time(1, SC_MS);

  std::vector<float> buf;
  int head = 0;     // next write position
  bool init = false;

  SC_CTOR(RingBufferSC) { SC_THREAD(run); }

  void ensure_init() {
    if (!init) {
      buf.assign(fft_size, 0.0f);
      head = 0;
      init = true;
    }
  }

  void run() {
    ensure_init();

    while (true) {
      SampleBlock b = in.read();
      ensure_init();

      // write block
      for (float x : b.s) {
        buf[head] = x;
        head++;
        if (head >= fft_size) head = 0;
      }

      // model latency
      wait(latency);

      // read_all: output fft_size samples ordered oldest->newest
      AudioWindow w;
      w.x.resize(fft_size);
      w.ts = b.ts;

      int start = head; // head is "oldest" after wrap-style write
      for (int i = 0; i < fft_size; i++) {
        w.x[i] = buf[(start + i) % fft_size];
      }

      out.write(w);
    }
  }
};

// ===================== FFT + Magnitude (KissFFT) =====================
// Exactly like your process_fft(): kiss_fftr -> magnitude.
SC_MODULE(FFTMagSC) {
  sc_fifo_in<AudioWindow> in;
  sc_fifo_out<Spectrum>   out;

  int fft_size = 1024;

  kiss_fftr_cfg cfg = nullptr;
  std::vector<kiss_fft_cpx> out_cpx;

  SC_CTOR(FFTMagSC) { SC_THREAD(run); }

  void ensure_cfg() {
    if (!cfg) {
      cfg = kiss_fftr_alloc(fft_size, 0, nullptr, nullptr);
      out_cpx.assign(fft_size / 2 + 1, kiss_fft_cpx{0, 0});
    }
  }

  void run() {
    ensure_cfg();

    while (true) {
      AudioWindow w = in.read();
      ensure_cfg();

      if ((int)w.x.size() != fft_size) w.x.resize(fft_size, 0.0f);

      kiss_fftr(cfg, w.x.data(), out_cpx.data());

      Spectrum sp;
      sp.ts = w.ts;
      sp.mag.resize(fft_size / 2 + 1);

      float max_mag = 0.0f;

      // magnitude like your code
      for (int i = 0; i < (int)sp.mag.size(); i++) {
        float re = out_cpx[i].r;
        float im = out_cpx[i].i;
        float mag = std::sqrt(re * re + im * im);
        sp.mag[i] = mag;
      }

      // mimic your display_spectrum: find max over bins 1..40 (ignore DC bin 0)
      for (int i = 1; i < 40 && i < (int)sp.mag.size(); i++) {
        if (sp.mag[i] > max_mag) max_mag = sp.mag[i];
      }
      sp.max_mag = max_mag;

      // bars normalized like display_spectrum (bins 1..40)
      sp.bars.fill(0.0f);
      for (int i = 1; i <= 40; i++) {
        if (i < (int)sp.mag.size()) {
          sp.bars[i - 1] = (max_mag > 0.0f) ? (sp.mag[i] / max_mag) : 0.0f; // [0..1]
        }
      }

      out.write(sp);
    }
  }

  ~FFTMagSC() override {
    if (cfg) kiss_fftr_free(cfg);
  }
};

// ===================== Probe: Spectrum -> sc_signal for VCD =====================
// We trace max_mag + 40 bars to visualize spectrum evolution in GTKWave.
SC_MODULE(SpectrumProbeSC) {
  sc_fifo_in<Spectrum> in;

  sc_out<float> max_mag_sig;
  sc_vector< sc_out<float> > bar_sig; // bar01..bar40

  SC_CTOR(SpectrumProbeSC)
    : bar_sig("bar_sig", 40)
  {
    SC_THREAD(run);
  }

  void run() {
    while (true) {
      Spectrum sp = in.read();
      max_mag_sig.write(sp.max_mag);
      for (int i = 0; i < 40; i++) {
        bar_sig[i].write(sp.bars[i]);
      }
    }
  }
};

// ===================== Optional console visual (debug) =====================
// Not required, but helpful to see it live without GTKWave.
SC_MODULE(ConsoleVizSC) {
  sc_in<float> max_mag;
  sc_vector< sc_in<float> > bars;

  int tick = 0;

  SC_CTOR(ConsoleVizSC)
    : bars("bars", 40)
  {
    SC_THREAD(run);
  }

  void run() {
    while (true) {
      wait(max_mag->value_changed_event());
      tick++;

      // print occasionally
      if (tick % 25 == 0) {
        std::cout << sc_time_stamp()
                  << " max_mag=" << max_mag.read()
                  << " bar1=" << bars[0].read()
                  << " bar5=" << bars[4].read()
                  << " bar10=" << bars[9].read()
                  << "\n";
      }
    }
  }
};

// ===================== sc_main =====================
int sc_main(int, char**) {
  // FIFOs
  sc_fifo<SampleBlock>  q_samples(4);
  sc_fifo<AudioWindow>  q_window(4);
  sc_fifo<Spectrum>     q_spec(4);

  // Signals to trace
  sc_signal<float> max_mag_sig("max_mag");

  sc_vector< sc_signal<float> > bar_sig;
  bar_sig.init(40);

  // Modules
  AudioSourceSC     src("src");
  RingBufferSC      rb("rb");
  FFTMagSC          fftmag("fftmag");
  SpectrumProbeSC   prob("probe");
  ConsoleVizSC      cv("console_viz");

  // Match your config style (MP_FFT_SIZE)
  rb.fft_size = 1024;
  fftmag.fft_size = 1024;

  // Wiring (architecture like your real code)
  src.out(q_samples);
  rb.in(q_samples);
  rb.out(q_window);
  fftmag.in(q_window);
  fftmag.out(q_spec);

  prob.in(q_spec);
  prob.max_mag_sig(max_mag_sig);
  for (int i = 0; i < 40; i++) prob.bar_sig[i](bar_sig[i]);

  cv.max_mag(max_mag_sig);
  for (int i = 0; i < 40; i++) cv.bars[i](bar_sig[i]);

  // VCD tracing
  sc_trace_file* tf = sc_create_vcd_trace_file("musicviz_arch");
  tf->set_time_unit(1, SC_MS);

  sc_trace(tf, max_mag_sig, "max_mag");

  // bars named bar01..bar40 in VCD
  for (int i = 0; i < 40; i++) {
    char name[16];
    std::snprintf(name, sizeof(name), "bar%02d", i + 1);
    sc_trace(tf, bar_sig[i], name);
  }

  sc_start(sc_time(5, SC_SEC));
  sc_close_vcd_trace_file(tf);
  return 0;
}
