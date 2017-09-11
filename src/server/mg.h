#ifndef K_MG_H_
#define K_MG_H_

namespace K {
int mgT = 0;
vector<mGWmt> mGWmt_;
json mGWmktFilter;
double mgFairValue = 0;
double mgEwmaL = 0;
double mgEwmaM = 0;
double mgEwmaS = 0;
double mgEwmaP = 0;
vector<double> mgSMA3;
vector<double> mgStatFV;
vector<double> mgStatBid;
vector<double> mgStatAsk;
vector<double> mgStatTop;
double mgStdevFV = 0;
double mgStdevFVMean = 0;
double mgStdevBid = 0;
double mgStdevBidMean = 0;
double mgStdevAsk = 0;
double mgStdevAskMean = 0;
double mgStdevTop = 0;
double mgStdevTopMean = 0;
double mgTargetPos = 0;
double mgStdevSMA3 = 0;
double mgStdevSMAMean = 0 ;
vector<double> mgSTDevStatSMA3;
vector<double> mgSMA3_temp;   // warm-up SMA3 data!
vector<double> mgWSMA33;          // Logging SMA3 values
vector<double> ArrayEwmaL;     // vector for EwmaL
vector<double> ArrayEwmaS;     // vector for EwmaS
vector<double> ArrayEwmaM;    // vector for EwmaM
vector<double> mgStdevSMA33; // stdev SMA3
double mgEwmaProfit = 0;
vector<int> mgMATIME;
double mgSMA3G;     // global SMA3 current value
class MG {
public:
static void main() {
        load();
        EV::on(mEv::MarketTradeGateway, [](json k) {
          tradeUp(k);
        });
        EV::on(mEv::MarketDataGateway, [](json o) {
          levelUp(o);
        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        UI::uiSnap(uiTXT::EWMAChart, &onSnapEwma);
};
static bool empty() {
        return (mGWmktFilter.is_null()
          or mGWmktFilter["bids"].is_null()
          or mGWmktFilter["asks"].is_null()
          or !mGWmktFilter["bids"].is_array()
          or !mGWmktFilter["asks"].is_array()
          or !mGWmktFilter["bids"].size()
          or !mGWmktFilter["asks"].size()
        );
      };
      static void calcStats() {
        if (++mgT == 60) {
                mgT = 0;
                ewmaPUp();
                ewmaUp();
        }
        stdevPUp();
};
static void calcFairValue() {
        if (empty()) return;
        double mgFairValue_ = mgFairValue;
        double topAskPrice = mGWmktFilter.value("/asks/0/price"_json_pointer, 0.0);
        double topBidPrice = mGWmktFilter.value("/bids/0/price"_json_pointer, 0.0);
        double topAskSize = mGWmktFilter.value("/asks/0/size"_json_pointer, 0.0);
        double topBidSize = mGWmktFilter.value("/bids/0/size"_json_pointer, 0.0);
        if (!topAskPrice or !topBidPrice or !topAskSize or !topBidSize) return;
        mgFairValue = FN::roundNearest(
          mFairValueModel::BBO == (mFairValueModel)QP::getInt("fvModel")
            ? (topAskPrice + topBidPrice) / 2
            : (topAskPrice * topAskSize + topBidPrice * topBidSize) / (topAskSize + topBidSize),
          gw->minTick
        );
        if (!mgFairValue or (mgFairValue_ and abs(mgFairValue - mgFairValue_) < gw->minTick)) return;
        EV::up(mEv::PositionGateway);
        UI::uiSend(uiTXT::FairValue, {{"price", mgFairValue}}, true);
};
private:
static void load() {
        json k = DB::load(uiTXT::MarketData);
        if (k.size()) {
          for (json::iterator it = k.begin(); it != k.end(); ++it) {
            if (it->value("time", (unsigned long)0)+QP::getInt("quotingStdevProtectionPeriods")*1e+3<FN::T()) continue;
            mgStatFV.push_back(it->value("fv", 0.0));
            mgStatBid.push_back(it->value("bid", 0.0));
            mgStatAsk.push_back(it->value("ask", 0.0));
            mgStatTop.push_back(it->value("bid", 0.0));
            mgStatTop.push_back(it->value("ask", 0.0));
          }
          calcStdev();
        }
        cout << FN::uiT() << "DB" << RWHITE << " loaded " << mgStatFV.size() << " STDEV Periods." << endl;
        k = DB::load(uiTXT::EWMAChart);
        if (k.size()) {
          k = k.at(0);
          if (k.value("time", (unsigned long)0)+QP::getInt("longEwmaPeriods")*6e+4>FN::T())
            mgEwmaL = k.value("ewmaLong", 0.0);
          if (k.value("time", (unsigned long)0)+QP::getInt("mediumEwmaPeriods")*6e+4>FN::T())
            mgEwmaM = k.value("ewmaMedium", 0.0);
          if (k.value("time", (unsigned long)0)+QP::getInt("shortEwmaPeriods")*6e+4>FN::T())
            mgEwmaS = k.value("ewmaShort", 0.0);
        }
        cout << FN::uiT() << "DB" << RWHITE << " loaded EWMA Long = " << mgEwmaL << "." << endl;
        cout << FN::uiT() << "DB" << RWHITE << " loaded EWMA Medium = " << mgEwmaM << "." << endl;
        cout << FN::uiT() << "DB" << RWHITE << " loaded EWMA Short = " << mgEwmaS << "." << endl;
      };
      static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
                k.push_back(tradeUp(mGWmt_[i]));
        return k;
};
static json onSnapFair(json z) {
        return {{{"price", mgFairValue}}};
};
static json onSnapEwma(json z) {
        return {{
                        {"stdevWidth", {
                                 {"fv", mgStdevFV},
                                 {"fvMean", mgStdevFVMean},
                                 {"tops", mgStdevTop},
                                 {"topsMean", mgStdevTopMean},
                                 {"bid", mgStdevBid},
                                 {"bidMean", mgStdevBidMean},
                                 {"ask", mgStdevAsk},
                                 {"askMean", mgStdevAskMean}
                         }},
                        {"ewmaQuote", mgEwmaP},
                        {"ewmaShort", mgEwmaS},
                        {"ewmaMedium", mgEwmaM},
                        {"ewmaLong", mgEwmaL},
                        {"fairValue", mgFairValue}
                }};
};
static void stdevPUp() {
        if (empty()) return;
        double topBid = mGWmktFilter.value("/bids/0/price"_json_pointer, 0.0);
        double topAsk = mGWmktFilter.value("/bids/0/price"_json_pointer, 0.0);
        if (!topBid or !topAsk) return;
        mgStatFV.push_back(mgFairValue);
        mgStatBid.push_back(topBid);
        mgStatAsk.push_back(topAsk);
        mgStatTop.push_back(topBid);
        mgStatTop.push_back(topAsk);
        calcStdev();
        DB::insert(uiTXT::MarketData, {
          {"fv", mgFairValue},
          {"bid", topBid},
          {"ask", topAsk},
          {"time", FN::T()},
        }, false, "NULL", FN::T() - 1e+3 * QP::getInt("quotingStdevProtectionPeriods"));
      };
      static void tradeUp(json k) {
        mGWmt t(
          gw->exchange,
          gw->base,
          gw->quote,
          k.value("price", 0.0),
          k.value("size", 0.0),
          FN::T(),
          (mSide)k.value("make_side", 0)
        );
        mGWmt_.push_back(t);
        if (mGWmt_.size()>69) mGWmt_.erase(mGWmt_.begin());
        EV::up(mEv::MarketTrade);
        UI::uiSend(uiTXT::MarketTrade, tradeUp(t));
};
static void levelUp(json k) {
        filter(k);
        UI::uiSend(uiTXT::MarketData, k, true);
};
static json tradeUp(mGWmt t) {
        json o = {
                {"exchange", (int)t.exchange},
                {"pair", {{"base", t.base}, {"quote", t.quote}}},
                {"price", t.price},
                {"size", t.size},
                {"time", t.time},
                {"make_size", (int)t.make_side}
        };
        return o;
      };
      static void ewmaUp() {
        calcEwma(&mgEwmaL, QP::getInt("longEwmaPeriods"));
        calcEwma(&mgEwmaM, QP::getInt("mediumEwmaPeriods"));
        calcEwma(&mgEwmaS, QP::getInt("shortEwmaPeriods"));
        calcTargetPos();
        calcASP();
        calcSafety();
        EV::up(mEv::PositionBroker);
        UI::uiSend(uiTXT::EWMAChart, {
                                {"stdevWidth", {
                                         {"fv", mgStdevFV},
                                         {"fvMean", mgStdevFVMean},
                                         {"tops", mgStdevTop},
                                         {"topsMean", mgStdevTopMean},
                                         {"bid", mgStdevBid},
                                         {"bidMean", mgStdevBidMean},
                                         {"ask", mgStdevAsk},
                                         {"askMean", mgStdevAskMean}
                                 }},
                                {"ewmaQuote", mgEwmaP},
                                {"ewmaShort", mgEwmaS},
                                {"ewmaMedium", mgEwmaM},
                                {"ewmaLong", mgEwmaL},
                                {"fairValue", mgFairValue}
                        }, true);
        DB::insert(uiTXT::EWMAChart, {
          {"ewmaLong", mgEwmaL},
          {"ewmaMedium", mgEwmaM},
          {"ewmaShort", mgEwmaS},
          {"time", FN::T()}
        });
      };
      static void ewmaPUp() {
        calcEwma(&mgEwmaP, QP::getInt("quotingEwmaProtectionPeriods"));
        EV::up(mEv::EWMAProtectionCalculator);
      };
      static void filter(json k) {
        mGWmktFilter = (k.is_null() or k["bids"].is_null() or k["asks"].is_null())
                       ? json{{"bids", {}},{"asks", {}}} : k;
        if (empty()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if (it->second["side"].is_number() and it->second["price"].is_number() and it->second["quantity"].is_number())
            filter(mSide::Bid == (mSide)it->second.value("side", 0) ? "bids" : "asks", it->second);
        if (!empty()) {
          calcFairValue();
          EV::up(mEv::FilteredMarket);
        }
      };
      static void filter(string k, json o) {
        for (json::iterator it = mGWmktFilter[k].begin(); it != mGWmktFilter[k].end();)
          if (abs(it->value("price", 0.0) - o.value("price", 0.0)) < gw->minTick) {
            (*it)["size"] = it->value("size", 0.0) - o.value("quantity", 0.0);
            if (it->value("size", 0.0) < gw->minTick) mGWmktFilter[k].erase(it);
            break;
          } else ++it;
      };
      static void cleanStdev() {
        size_t periods = (size_t)QP::getInt("quotingStdevProtectionPeriods");
        if (mgStatFV.size()>periods) mgStatFV.erase(mgStatFV.begin(), mgStatFV.end()-periods);
        if (mgStatBid.size()>periods) mgStatBid.erase(mgStatBid.begin(), mgStatBid.end()-periods);
        if (mgStatAsk.size()>periods) mgStatAsk.erase(mgStatAsk.begin(), mgStatAsk.end()-periods);
        if (mgStatTop.size()>periods*2) mgStatTop.erase(mgStatTop.begin(), mgStatTop.end()-(periods*2));
};
static void calcStdev() {
        cleanStdev();
        if (mgStatFV.size() < 2 or mgStatBid.size() < 2 or mgStatAsk.size() < 2 or mgStatTop.size() < 4) return;
        double k = QP::getDouble("quotingStdevProtectionFactor");
        mgStdevFV = calcStdev(mgStatFV, k, &mgStdevFVMean);
        mgStdevBid = calcStdev(mgStatBid, k, &mgStdevBidMean);
        mgStdevAsk = calcStdev(mgStatAsk, k, &mgStdevAskMean);
        mgStdevTop = calcStdev(mgStatTop, k, &mgStdevTopMean);

};
static double calcStdev(vector<double> a, double f, double *mean) {
        int n = a.size();
        if (n == 0) return 0.0;
        double sum = 0;
        for (int i = 0; i < n; ++i) sum += a[i];
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (int i = 0; i < n; ++i) {
                double diff = a[i] - *mean;
                sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * f;
};
static void calcEwma(double *k, int periods) {
        if (*k) {
                double alpha = (double)2 / (periods + 1);
                *k = alpha * mgFairValue + (1 - alpha) * *k;
        } else *k = mgFairValue;
};
static void calcTargetPos() {
        ;
        double SMA3 = 0;
        if(mgSMA3.size() == 0)
        {
                cout << FN::uiT()  << " Warming up SMA3" << "\n";
                vector <double> preLoadSMA = LoadSMA(QP::getInt("safetytime")*10);

                for (vector<double>::iterator ia = preLoadSMA.begin(); ia != preLoadSMA.end(); ++ia)
                {
                        cout << "SMAAAA: " << *ia << "\n";
                        mgSMA3.push_back(*ia);

                        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
                        SMA3 = 0;

                        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
                        {
                                SMA3 += *it;
                        }
                                SMA3 /= mgSMA3.size();
                                mgSMA3G = SMA3;
                                mgStdevSMA3 = calcStdev(mgStdevSMA33, QP::getDouble("quotingStdevProtectionFactor"), &mgStdevSMAMean);
                                mgWSMA33.push_back(mgSMA3G);
                                mgStdevSMA33.push_back(mgStdevSMA3);

                }

        } else {

                mgSMA3.push_back(mgFairValue);
                if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
                SMA3 = 0;

                for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
                        SMA3 += *it;
                SMA3 /= mgSMA3.size();
                mgSMA3G = SMA3;
                mgStdevSMA3 = calcStdev(mgStdevSMA33, QP::getDouble("quotingStdevProtectionFactor"), &mgStdevSMAMean);
                mgWSMA33.push_back(mgSMA3G);
                mgStdevSMA33.push_back(mgStdevSMA3);
        }
        cout << FN::uiT() << "SMA3 StDev: " << mgStdevSMA3 << "\n";
        double newTargetPosition = 0;
        if(QP::getBool("take_profit_active") ) {
                /*If ewma take profit > SMA3
                        newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qp["ewmaSensiblityPercentage"]
                        else
                        If ewma take profit < SMA3
                        new TargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qp["ewmaSensiblityPercentage"] - Take_profit_calc.
                 */
                cout << FN::uiT()  << "mgEwmaProfit: " << mgEwmaProfit << " SMA3: " << SMA3 << "\n";
                double takeProfit = (((qp["take_profic_percent"].get<double>()/100) * 2) / 100) - 1;
                if(mgEwmaProfit > SMA3) {
                        newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qp["ewmaSensiblityPercentage"].get<double>());

                        cout << FN::uiT()  << "EWMA Profit  > SMA3 " << mgEwmaProfit << " | " << SMA3  << " Target: " << newTargetPosition <<  "\n";
                        cout << FN::uiT()  << "EWMA Profit Take Profit: " << takeProfit << "\n";
                        qp["takeProfitNow"] = false;
                }
                if(mgEwmaProfit < SMA3) {
                        newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / (qp["ewmaSensiblityPercentage"].get<double>() - takeProfit) );
                        cout << FN::uiT()  << "EWMA Profit  < SMA3 " << mgEwmaProfit << " | " << SMA3  << " Target: " << newTargetPosition <<  "\n";
                        cout << FN::uiT()  << "EWMA Profit Take Profit: " << takeProfit << "\n";
                        qp["takeProfitNow"] = true;
                        cout << FN::uiT()  << " Should take a small profit now\n";

                }


        } else {
          if ((mAutoPositionMode)QP::getInt("autoPositionMode") == mAutoPositionMode::EWMA_LMS) {
            double newTrend = ((SMA3 * 100 / mgEwmaL) - 100);
            double newEwmacrossing = ((mgEwmaS * 100 / mgEwmaM) - 100);
            newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / QP::getDouble("ewmaSensiblityPercentage"));
          } else if ((mAutoPositionMode)QP::getInt("autoPositionMode") == mAutoPositionMode::EWMA_LS)
            newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / QP::getDouble("ewmaSensiblityPercentage"));

        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;

        mgTargetPos = newTargetPosition;
};


static void calcASP() {
        qp["aspvalue"] = ((mgEwmaS * 100/ mgEwmaL) - 100);
        cout << FN::uiT()  <<  "ASP Evaluation: " << ((mgEwmaS * 100/ mgEwmaL) - 100) << "\n";
        cout << FN::uiT() <<  "ASP Evaluation: fairV: " << mgFairValue << "\n";
        cout << FN::uiT() <<  "ASP Evaluation: SMA3 Latest: " << mgSMA3G << "\n";
        cout << FN::uiT() << "Current Short: " << mgEwmaS << "\n";
        cout << FN::uiT() << "Current Long: " << mgEwmaL << "\n";
        if(QP::getBool("take_profit_active") ) {
                if(mgEwmaProfit < mgSMA3G)
                {
                        qp["asptriggered"] = false;
                        cout << "Disabling ASP for TP\n";
                        return;
                }
        }
        if (
                (
                        (
                                (
                                        mgEwmaS < mgSMA3G
                                )
                                and
                                (
                                        (
                                                qp["aspvalue"].get<double>() >= qp["asp_high"].get<double>()
                                        )
                                )
                        )
                        ||
                        (
                                (
                                        qp["aspvalue"].get<double>() <= qp["asp_low"].get<double>()
                                )
                        )
                )
                && qp["aspactive"].get<bool>() == true
                ) {
                //  cout << "ASP high?: " << qp["aspvalue"].get<double>() " >= " << qp["asp_high"].get<double>() << " or asp low?: " << qp["aspvalue"].get<double>() << "<= " << qp["asp_low"].get<double>()) << "\n";
                if ( qp["asptriggered"].get<bool>() == false)
                {
                        qp["asptriggered"] = true;
                        cout << FN::uiT() << "ASP has been activated! pDiv should be set to Zero!\n";
                }
                cout << FN::uiT() << "ASP Active! pDiv should be set to Zero!\n";
        } else {
                if(qp["aspactive"].get<bool>() == true && qp["asptriggered"].get<bool>() == true)
                {
                        qp["asptriggered"] = false;
                        cout << FN::uiT() << "ASP Deactivated" << "\n";
                }
        }


}
static void calcSafety() {
        //  unsigned long int SMA33STARTTIME = std::time(nullptr); // get the time since EWMAProtectionCalculator
        //mgMATIME.push_back(std::time(nullptr));
        //if (mgMATIME.size()>1000) mgMATIME.erase(mgMATIME.begin(), mgMATIME.end()-1);
        // lets make a SMA logging average

        cout << FN::uiT() << "Safety Active: " << qp["safetyactive"].get<bool>() << "\n";
        cout << FN::uiT() << "Safety Enabled:" << qp["safetynet"].get<bool>() << "\n";

        cout << FN::uiT() << "Safety SMA3 Array size: " << mgWSMA33.size() << "\n";
        if (mgWSMA33.size()>1000) mgWSMA33.erase(mgWSMA33.begin(), mgWSMA33.end()-1);

        if (qp["safetynet"].get<bool>() == false) {
                qp["safemode"]  = (int)mSafeMode::unknown;
                qp["safetyactive"] = false;
                return;
        }


        if((signed)mgWSMA33.size() > qp["safetytime"].get<signed int>()) {
                cout << FN::uiT() << "Entering Safety Check, SMA3Log Array Size: " << mgWSMA33.size()  << " and safetme index is: " << qp["safetytime"].get<int>() << "\n";
                cout << FN::uiT() << "Latest SMA3 Average in deck " << mgWSMA33.back() << " Target SMA3 " << qp["safetytime"].get<int>() << "  Indexes BEHIND is " << mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) << "\n";
                //cout << FN::uiT() << "Latest SMA3 TIME in deck " << mgMATIME.back() << " Target SMA3 TIME " << qp["safetytime"].get<int>() << "  Indexes BEHIND is " << mgMATIME.at(mgMATIME.size() - (qp["safetytime"].get<int>()+1)) << "\n";

                if (
                        (
                                (
                                        (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) ) - 100
                                )
                                >
                                (
                                        qp["safetyP"].get<double>()/100
                                )
                        )
                        &&  !qp["safetyactive"].get<bool>()           // make sure we are not already in a safety active state
                        &&  qp["safetynet"].get<bool>()           // make sure safey checkbox is active on UI

                        )
                {
                        //  printf("debug12\n");
                        // activate Safety, Safety buySize
                        double SafeBuyValuation =   (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) - 100);
                        qp["safemode"] = (int)mSafeMode::buy;
                        cout << FN::uiT() << "Setting SafeMode: "<< qp["safemode"].get<int>() << " ENUM Value: " << (int)mSafeMode::buy << "\n";
                        qp["safetyactive"] = true;
                        qp["safetimestart"] = std::time(nullptr);
                        qp["safetyduration"] = qp["safetimestart"].get<int>() + (qp["safetimeOver"].get<int>() * 60000);
                        cout << FN::uiT() << "Activating Safety BUY Mode First SMA3: " << mgWSMA33.back() << " SMA3[index -" <<  qp["safetytime"].get<int>() << "] Value is: " << mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) << " Equals: " << SafeBuyValuation << " which is More than safety Percent: " << qp["safetyP"].get<double>()/100 << "\n";

                }
/*
                if (     (
                                 (
                                         (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) ) - 100
                                 )
                                 <
                                 -1 * (qp["safetyP"].get<double>()/100)
                                 )
                         &&   !qp["safetyactive"].get<bool>()          // make sure we are not already in a safety active state
                         &&   qp["safetynet"].get<bool>()          // make sure safey checkbox is active on UI

                         )
                {
                        double SafeSellValuation = (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) - 100);
                        qp["safemode"] = (int)mSafeMode::sell;
                        cout << "Setting SafeMode: "<< qp["safemode"].get<int>() << " ENUM Value: " << (int)mSafeMode::sell << "\n";
                        qp["safetyactive"] = true;
                        qp["safetimestart"] = std::time(nullptr);
                        qp["safetyduration"] = qp["safetimestart"].get<int>() + (qp["safetimeOver"].get<int>() * 60000);
                        cout << "Activating Safety SELL Mode First SMA3: " << mgWSMA33.back() << " SMA3[index -" <<  qp["safetytime"].get<int>() <<"] Value is: " << mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) << " Equals: " << SafeSellValuation << " which is less than safety Percent: " << -1 * (qp["safetyP"].get<double>()/100) << "\n";
                        cout << "Safety Duration period is: " << qp["safetyduration"].get<unsigned long int>() << "started at: " << qp["safetimestart"].get<unsigned long int>() << " \n";
                }
 */
        }
        if(qp["safetyactive"].get<bool>() == true and qp["safetynet"].get<bool>() == true)
        {
                if (qp["safemode"].get<int>() == (int)mSafeMode::sell) {
                        cout << FN::uiT() << "SAFETY! Safe Mode Selling! " << "\n";
                }
                if(qp["safemode"].get<int>() == (int)mSafeMode::buy) {
                        cout << FN::uiT() << "SAFETY! Safe Mode Buying! " << "\n";
                }
                if(qp["safemode"].get<int>() == (int)mSafeMode::unknown) {
                        cout << FN::uiT() << "SAFETY! Safe Mode unknown!! " << "\n";
                }
                cout << FN::uiT() << "SAFETY! " << " pDiv should now be set to ZERO.\n";
                cout << FN::uiT() << "SAFETY! " << qp["safemode"].get<int>() << "\n";

        }
        // check for safety time is over
        if((signed)mgWSMA33.size() > qp["safetytime"].get<signed int>() ) {
                cout << FN::uiT() << "Entering Safety Check EXIT, SMA3Log Array Size: " << mgWSMA33.size()  << " and safetme index is: " << qp["safetytime"].get<int>() << "\n";

                if(qp["safetyactive"].get<bool>() and qp["safetynet"].get<bool>())          // Check to make sure we ae currently in active safety state and safety box in UI is active
                {

                        cout << FN::uiT() << "is Timer Over?:" << "\n";
                        cout << FN::uiT() << "Current Value: " << mgWSMA33.back() << "\n";
                        cout << FN::uiT() << "Index back: " << mgWSMA33.at(mgWSMA33.size() - qp["safetytime"].get<int>()) << "\n";

                        //cout << FN::uiT() << "Current SMA3 time: " << mgMATIME.back() << "\n";
                        cout << FN::uiT() << "time counter: " <<   qp["safetimestart"] << "\n";
                        cout << FN::uiT() << "time Difference: " << difftime(std::time(nullptr),(qp["safetimestart"].get<double>())) << "\n";
                        cout << FN::uiT() << "Duration: " << (qp["safetimeOver"].get<int>() * 60) << "\n";
                        // exit sell mode
                        if(     (mgWSMA33.back() > mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) )
                                && (mSafeMode)qp["safemode"].get<int>() == mSafeMode::sell
                                ) {
                                cout<< FN::uiT()  << "SAFETY: Breaking Safety Mode\n";
                                qp["safemode"] = (int)mSafeMode::unknown;
                                qp["safetyactive"] = false;
                                cout << FN::uiT() << "SAFETY: Exit Sell Mode\n";

                        }
                        if(     (mgWSMA33.back() < mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) )
                                && (mSafeMode)qp["safemode"].get<int>() == mSafeMode::buy
                                ) {
                                cout << FN::uiT() << "SAFETY: Breaking Safety Mode\n";
                                qp["safemode"] = (int)mSafeMode::unknown;
                                qp["safetyactive"] = false;
                                cout << FN::uiT() <<  "SAFETY: Exit Buy Mode\n";

                        }
                        //  printf("debugzz\n");
                        //  double spacer = mGWSMA33.at(mGWSMA33.size() - qp["safetytime"].get<double>()).get<double>();
                        //if( (mGWSMA33.back() > mGWSMA33.at(mGWSMA33.size() - qp["safetytime"].get<int>()) ) && (qp["safetyduration"].get<unsigned long int>() >= (qp["safetimeOver"].get<unsigned long int>() * 60000)))
                        /*if(



                                difftime(std::time(nullptr),(qp["safetimestart"].get<double>()))

                                >=

                                (qp["safetyduration"].get<double>() * 60)





                                )
                           {
                                cout << "Breaking Safey SELL Mode Time over:" << (qp["safetimeOver"].get<int>() * 60) << " was greater than " << difftime(std::time(nullptr),(qp["safetimestart"].get<double>())) << "\n";
                                cout << "Breaking Safey SELL Mode: Latest SMA3 Value: " << mgWSMA33.back() << " was Greater than " << mgWSMA33.at(mgWSMA33.size() - (qp["safetytime"].get<int>()+1)) << "\n";
                                qp["safemode"] = (int)mSafeMode::unknown;
                                qp["safetyactive"] = false;
                                cout << "Exiting safety mode sell\n";
                           }
                         */
                }
        }
        // Set newTargetPosition

        if( qp["safetyactive"].get<bool>() && qp["safetynet"].get<bool>() && (mSafeMode)qp["safemode"].get<int>() == mSafeMode::buy)
        {
                mgTargetPos = 1;
                cout << "newTargetPosition activated to: " << mgTargetPos << "via Safety buy Action\n";
        }
        if( mgEwmaL > mgEwmaS )
        {
                mgTargetPos = -1;
                cout << "newTargetPosition activated to: " << mgTargetPos << "via Safety sell Liquidate it ALL Action\n";
                cout << FN::uiT() << "Long: " << mgEwmaL << "\n";
                cout << FN::uiT() << "Short: " << mgEwmaS << "\n";
                cout << FN::uiT() << "SMA3: " << mgSMA3G << "\n";
        }





}

static double LoadEWMA(int periods) {
        //  string baseurl = "https://api.cryptowat.ch/markets/bitfinex/ltcusd/ohlc?periods=60";
        cout << FN::uiT()  << "Starting EWMA\n";
        string baseurl = "http://34.227.139.87/MarketPublish/";
        //string baseurl = "http://34.227.139.87/MarketPublish/fairV.php";
        string pair =  CF::cfString("TradedPair");
        pair.erase(std::remove(pair.begin(), pair.end(), '/'), pair.end());
        cout << FN::uiT()  << "pair: " << pair << "\n";
        string exchange =  CF::cfString("EXCHANGE");
        int CurrentTime = std::time(nullptr);
        int doublePeriods = periods * 5;
        int BackTraceStart = CurrentTime - (periods * 60000);
        std::vector<double> EWMAArray;
        vector<double> EMAStorage;
        double myEWMA = 0;
        double tempEWMA = 0;
        double previous = 0;
        bool first = true;
        string fullURL = string(baseurl.append("?periods=").append(std::to_string(doublePeriods)).append("&exchange=").append(exchange).append("&pair=").append(pair));
        cout << FN::uiT()  << "Full URL: " << fullURL << "\n";
        json EWMA = FN::wJet(fullURL);
        //cout << EWMA << "\n";
        for (auto it = EWMA["result"][std::to_string(doublePeriods)].begin(); it != EWMA["result"][std::to_string(doublePeriods)].end(); ++it)
        {

                json EMAArray = it.value();

                //EMAStorage.push_back(EMAArray["FairValue"].get<double>());
                EMAStorage.push_back(EMAArray[1].get<double>());

        }
        std::reverse(std::begin(EMAStorage), std::end(EMAStorage));
        for(auto it = EMAStorage.begin(); it != EMAStorage.end(); ++it)
        {
                myEWMA = *it;
                //  myEWMA = MycalcEwma(*it, previous,periods);
                tempEWMA = zEwma(myEWMA,tempEWMA,periods);
                //cout << "Close Value is: " << *it << "\n";

        }
        cout << FN::uiT()  << "period: " << periods << " EWMA is: " << tempEWMA << "\n";
        return tempEWMA;
}

static double zEwma(double k, double previous, int periods) {
        if (previous) {
                double alpha = (double)2 / (periods + 1);
                k = alpha * k + (1 - alpha) * previous;
        } else k = k;
        return k;
};

static vector <double> LoadSMA(int periods) {
        //  string baseurl = "https://api.cryptowat.ch/markets/bitfinex/ltcusd/ohlc?periods=60";
        cout << FN::uiT()  << "Starting Load SMA\n";
        //string baseurl = "http://34.227.139.87/MarketPublish/";
        string baseurl = "http://34.227.139.87/MarketPublish/fairV.php";
        string pair =  CF::cfString("TradedPair");
        pair.erase(std::remove(pair.begin(), pair.end(), '/'), pair.end());
        cout << FN::uiT()  << "pair: " << pair << "\n";
        string exchange =  CF::cfString("EXCHANGE");
        int CurrentTime = std::time(nullptr);
        int doublePeriods = periods;
        int BackTraceStart = CurrentTime - (periods * 60000);
        std::vector<double> EWMAArray;
        vector<double> EMAStorage;
        double myEWMA = 0;
        double previous = 0;
        bool first = true;
        string fullURL = string(baseurl.append("?periods=").append(std::to_string(doublePeriods)).append("&exchange=").append(exchange).append("&pair=").append(pair));
        cout << FN::uiT()  << "Full URL: " << fullURL << "\n";
        json EWMA = FN::wJet(fullURL);
        //cout << EWMA << "\n";
        for (auto it = EWMA["result"][std::to_string(doublePeriods)].begin(); it != EWMA["result"][std::to_string(doublePeriods)].end(); ++it)
        {

                json EMAArray = it.value();

                EMAStorage.push_back(EMAArray["FairValue"].get<double>());

        }
        std::reverse(std::begin(EMAStorage), std::end(EMAStorage));

        return EMAStorage;
}


};
}

#endif
