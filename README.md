**Dogecoin Breakout Strategy Backtest**
C++ backtest of a simple breakout trading strategy on Dogecoin price data. 
Compares strategy performance against buy-and-hold benchmark with interactive Chart.js visualisation.

** Logic**
- Entry: Buy when price makes a new N-day high (default: 50 days)
- Exit: Sell when price falls X% from entry price (default: 30%)
- Position: Single position at a time
- Capital: Fixed USD starting capital (default: $10k)
- Benchmark: Buy-and-hold from same start date

** Results**
![Strategy vs Buy & Hold](equity_full.html)

** Demo**
g++ -std=c++17 -O2 -o doge_backtest doge_backtest.cpp
./doge_backtest doge_price_2.csv equity_full

** Outputs:**
- `equity_full.csv` – Daily equity curves (strategy + buy/hold)
- `equity_full_data.json` – Chart data
- `equity_full.html` – [Open in browser](equity_full.html)

** Customise Parameters**
Edit these in `main()` to change 
int LOOKBACK = 50; nth day high lookback
double STOP_LOSS = 0.3; 30% stop loss from entry
double START_CAPITAL = 10000.0; // initial starting in USD

** Tech **
- C++17 – Fast backtesting engine
- Custom CSV parser – Handles `,` or `;` delimiters
- Chart.js v4 – Interactive equity curve visualisation
- No external dependencies – Single-file, self-contained

** Data Format**
Expects CSV with columns `snapped_at` (date) and `price` (USD):

** Features**
- Robust CSV parsing (auto-detects delimiter, case-insensitive headers)
- Production-grade error handling
- Memory efficient (single-pass processing)
- Reproducible results
- Browser-ready visualisation

** Build & Run**
macOS/Linux: `g++ -std=c++17`
Windows (WSL/WSL2): Same command  
VS Code: Built-in terminal works
