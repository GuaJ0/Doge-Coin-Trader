#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

//data stuff

struct Bar {
    string date;
    double close;
};

struct EquityPoint {
    string date;
    double strat;
    double buyhold;
};

// read doge_price csv file

vector<Bar> load_csv(const string& filename) {
    ifstream f(filename);
    vector<Bar> data;
    if (!f.is_open()) {
        cerr << "Cannot open " << filename << "\n";
        return data;
    }

    cerr << "Opening: [" << filename << "]\n";

    string header;
    if (!getline(f, header)) {
        cerr << "Empty file when reading header\n";
        return data;
    }

    cerr << "Raw header: [" << header << "]\n";

    // separate ,  ;
    char delim = (header.find(',') != string::npos) ? ',' : ';';

    auto split = [&](const string& s) {
        vector<string> out;
        string cur;
        for (char c : s) {
            if (c == delim) {
                out.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        out.push_back(cur);
        return out;
    };

    auto cols = split(header);
    int idxDate = -1, idxClose = -1;
    for (int i = 0; i < (int)cols.size(); ++i) {
        string c = cols[i];
        for (char& x : c) x = tolower(x);
        if (c == "snapped_at") idxDate = i;   // date column
        if (c == "price")      idxClose = i;  // price column
    }

    if (idxDate < 0 || idxClose < 0) {
        cerr << "Could not find Date/Close columns in header: "
             << header << "\n";
        return data;
    }

    auto trim = [](string s) {
        while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
        while (!s.empty() && isspace((unsigned char)s.front())) s.erase(s.begin());
        return s;
    };

    // for scientific notation 
    auto cleannum = [&](string s) {
        s = trim(s);
        string t;
        for (char c : s) {
            if ((c >= '0' && c <= '9') ||
                c == '.' || c == '-' || c == '+' ||
                c == 'e' || c == 'E')
            {
                t.push_back(c);
            }
        }
        return t;
    };

    string line;
    size_t parsed = 0, skipped = 0;
    while (getline(f, line)) {
        if (line.empty()) continue;
        auto row = split(line);
        if ((int)row.size() <= max(idxDate, idxClose)) {
            skipped++;
            continue;
        }
        Bar b;
        b.date = trim(row[idxDate]);
        try {
            b.close = stod(cleannum(row[idxClose]));
        } catch (...) {
            skipped++;
            continue;
        }
        data.push_back(b);
        parsed++;
    }

    cerr << "Loaded " << parsed << " rows, skipped " << skipped << "\n";
    return data;
}

// strategy
// breakout_lookback: N-day high to trigger entry (50)
//stop_pct: simple stop from ENTRY price (0.3  30%)
// fee_bps: trading cost in basis points

vector<EquityPoint> backtest(const vector<Bar>& data,
                             double start_capital    = 10000.0,
                             int    breakout_lookback = 50,
                             double stop_pct          = 0.30,
                             double fee_bps           = 10.0) {

    vector<EquityPoint> out;
    if ((int)data.size() <= breakout_lookback) return out;

    double cash    = start_capital;
    double qty     = 0.0;
    bool   holding = false;
    double entry_px = 0.0;   // entry price

    // benchmarj buy once at start of backtest window
    double bh_qty = start_capital / data[breakout_lookback].close;

    auto fee = [&](double notional) {
        return notional * (fee_bps / 10000.0);
    };

    for (int i = breakout_lookback; i < (int)data.size(); ++i) {
        double px = data[i].close;

        // nth day high for breakout
        double hi = 0.0;
        for (int k = i - breakout_lookback; k < i; ++k)
            hi = max(hi, data[k].close);

        // enter. price makes new nth day high and we are flat
        if (!holding && px > hi) {
            double notional = cash;
            double cost = fee(notional);
            if (notional > cost) {
                qty = (notional - cost) / px;
                cash = 0.0;
                holding = true;
                entry_px = px;
            }
        }
        // exit.  stop from entry price
        else if (holding && px < entry_px * (1.0 - stop_pct)) {
            double notional = qty * px;
            double cost = fee(notional);
            cash = max(0.0, notional - cost);
            qty = 0.0;
            holding = false;
            entry_px = 0.0;
        }

        double strat_equity = cash + qty * px;
        double bh_equity    = bh_qty * px;

        out.push_back({data[i].date, strat_equity, bh_equity});
    }

    return out;
}

// output

void write_equity_csv(const string& out_prefix,
                      const vector<EquityPoint>& eq) {
    string fname = out_prefix + ".csv";
    ofstream f(fname);
    f << "Date,Strategy,BH\n";
    for (const auto& e : eq) {
        f << e.date << "," << fixed << setprecision(2)
          << e.strat << "," << e.buyhold << "\n";
    }
    cerr << "Wrote " << fname << "\n";
}

void write_json(const string& out_prefix,
                const vector<EquityPoint>& eq) {
    string fname = out_prefix + "_data.json";
    ofstream f(fname);
    f << "{\n";

    f << "  \"labels\": [";
    for (size_t i = 0; i < eq.size(); ++i) {
        if (i) f << ",";
        f << "\"" << eq[i].date << "\"";
    }
    f << "],\n";

    f << "  \"strategy\": [";
    for (size_t i = 0; i < eq.size(); ++i) {
        if (i) f << ",";
        f << fixed << setprecision(2) << eq[i].strat;
    }
    f << "],\n";

    f << "  \"buyhold\": [";
    for (size_t i = 0; i < eq.size(); ++i) {
        if (i) f << ",";
        f << fixed << setprecision(2) << eq[i].buyhold;
    }
    f << "]\n";

    f << "}\n";
    cerr << "Wrote " << fname << "\n";
}
    
// main!!

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " doge_price_2.csv out_prefix\n";
        return 1;
    }
    string csv_file   = argv[1];
    string out_prefix = argv[2];

    auto data = load_csv(csv_file);
    if (data.size() < 100) {
        cerr << "Not enough rows in CSV\n";
        return 1;
    }

    auto equity = backtest(data);
    if (equity.empty()) {
        cerr << "Equity series is empty\n";
        return 1;
    }

    write_equity_csv(out_prefix, equity);
    write_json(out_prefix, equity);

    cerr << "Done.\n";
    return 0;
}
