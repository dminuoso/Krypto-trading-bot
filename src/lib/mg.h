#ifndef K_MG_H_
#define K_MG_H_

namespace K {
int mgT = 0;
vector<mGWmt> mGWmt_;
json mGWmkt;
json mGWmktFilter;
double mgFairValue = 0;
double mgEwmaL = 0;
double mgEwmaM = 0;
double mgEwmaS = 0;
double mgEwmaP = 0;
vector<double> mgSMA3;
vector<double> mgSMA3_temp; // warm-up SMA3 data!
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
vector<double> mgWSMA33;        // Logging SMA3 values
vector<double> ArrayEwmaL;   // vector for EwmaL
vector<double> ArrayEwmaS;   // vector for EwmaS
vector<double> ArrayEwmaM;  // vector for EwmaM
double mgEwmaProfit = 0;
vector<int> mgMATIME;
double mgSMA3G;   // global SMA3 current value
class MG {
public:
static void main(Local<Object> exports) {

        load();
        EV::evOn("MarketTradeGateway", [](json k) {
                                tradeUp(k);
                        });
        EV::evOn("MarketDataGateway", [](json o) {
                                levelUp(o);
                        });
        EV::evOn("GatewayMarketConnect", [](json k) {
                                if ((mConnectivity)k["/0"_json_pointer].get<int>() == mConnectivity::Disconnected)
                                        levelUp({});
                        });
        EV::evOn("QuotingParameters", [](json k) {
                                fairV();
                        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        UI::uiSnap(uiTXT::EWMAChart, &onSnapEwma);
};
static void calc() {
        if (++mgT == 60) {
                mgT = 0;
                ewmaPUp();
                ewmaUp();
        }
        stdevPUp();
};
static bool empty() {
        return (mGWmktFilter.is_null()
                or mGWmktFilter["bids"].is_null()
                or mGWmktFilter["asks"].is_null()
                );
};
private:
static void load() {
        json k = DB::load(uiTXT::MarketData);
        if (k.size()) {
                for (json::iterator it = k.begin(); it != k.end(); ++it) {
                        if ((*it)["time"].is_number() and (*it)["time"].get<unsigned long>()+qpRepo["quotingStdevProtectionPeriods"].get<int>()*1e+3<FN::T()) continue;
                        mgStatFV.push_back((*it)["fv"].get<double>());
                        mgStatBid.push_back((*it)["bid"].get<double>());
                        mgStatAsk.push_back((*it)["ask"].get<double>());
                        mgStatTop.push_back((*it)["bid"].get<double>());
                        mgStatTop.push_back((*it)["ask"].get<double>());
                }
                calcStdev();
        }
        cout << FN::uiT() << "DB loaded " << mgStatFV.size() << " STDEV Periods." << endl;
        k = DB::load(uiTXT::EWMAChart);
        if (k.size()) {
                if (k["/0/ewmaLong"_json_pointer].is_number() and (!k["/0/time"_json_pointer].is_number() or k["/0/time"_json_pointer].get<unsigned long>()+qpRepo["longEwmaPeriods"].get<int>()*6e+4>FN::T()))
                        mgEwmaL = k["/0/ewmaLong"_json_pointer].get<double>();
                if (k["/0/ewmaMedium"_json_pointer].is_number() and (!k["/0/time"_json_pointer].is_number() or k["/0/time"_json_pointer].get<unsigned long>()+qpRepo["mediumEwmaPeriods"].get<int>()*6e+4>FN::T()))
                        mgEwmaM = k["/0/ewmaMedium"_json_pointer].get<double>();
                if (k["/0/ewmaShort"_json_pointer].is_number() and (!k["/0/time"_json_pointer].is_number() or k["/0/time"_json_pointer].get<unsigned long>()+qpRepo["shortEwmaPeriods"].get<int>()*6e+4>FN::T()))
                        mgEwmaS = k["/0/ewmaShort"_json_pointer].get<double>();
        }
        cout << "Loading new EWMA\n";
        mgEwmaL = LoadEWMA(qpRepo["longEwmaPeriods"].get<int>());
        mgEwmaM = LoadEWMA(qpRepo["mediumEwmaPeriods"].get<int>());
        mgEwmaS = LoadEWMA(qpRepo["shortEwmaPeriods"].get<int>());
        qpRepo["_old_longEwmaPeriods"] = qpRepo["longEwmaPeriods"].get<int>(); // setting on start EWMA periods to _old_ place holders incase change during bot active
        qpRepo["_old_mediumEwmaPeriods"] = qpRepo["mediumEwmaPeriods"].get<int>();
        qpRepo["_old_shortEwmaPeriods"] = qpRepo["shortEwmaPeriods"].get<int>();
        cout << FN::uiT() << "DB loaded EWMA Long = " << mgEwmaL << "." << endl;
        cout << FN::uiT() << "DB loaded EWMA Medium = " << mgEwmaM << "." << endl;
        cout << FN::uiT() << "DB loaded EWMA Short = " << mgEwmaS << "." << endl;
        if(qpRepo["take_profit_active"].get<bool>()) { mgEwmaProfit = LoadEWMA(qpRepo["ewmaProfit"].get<int>()); }
        qpRepo["safemode"]  = (int)mSafeMode::unknown;
        qpRepo["takeProfitNow"] = false;
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
        mgStatFV.push_back(mgFairValue);
        mgStatBid.push_back(mGWmktFilter["/bids/0/price"_json_pointer].get<double>());
        mgStatAsk.push_back(mGWmktFilter["/asks/0/price"_json_pointer].get<double>());
        mgStatTop.push_back(mGWmktFilter["/bids/0/price"_json_pointer].get<double>());
        mgStatTop.push_back(mGWmktFilter["/asks/0/price"_json_pointer].get<double>());
        calcStdev();
        DB::insert(uiTXT::MarketData, {
                                {"fv", mgFairValue},
                                {"bid", mGWmktFilter["/bids/0/price"_json_pointer].get<double>()},
                                {"ask", mGWmktFilter["/bids/0/price"_json_pointer].get<double>()},
                                {"time", FN::T()},
                        }, false, "NULL", FN::T() - 1e+3 * qpRepo["quotingStdevProtectionPeriods"].get<int>());
};
static void tradeUp(json k) {

        mGWmt t(
                gw->exchange,
                gw->base,
                gw->quote,
                k["price"].get<double>(),
                k["size"].get<double>(),
                FN::T(),
                (mSide)k["make_side"].get<int>()
                );
        mGWmt_.push_back(t);
        if (mGWmt_.size()>69) mGWmt_.erase(mGWmt_.begin());
        EV::evUp("MarketTrade");
        UI::uiSend(uiTXT::MarketTrade, tradeUp(t));
};
static void levelUp(json k) {
        mGWmkt = k;
        filter();
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
        /*
           mgEwmaL = LoadEWMA(qpRepo["longEwmaPeriods"].get<int>());
           mgEwmaM = LoadEWMA(qpRepo["mediumEwmaPeriods"].get<int>());
           mgEwmaS = LoadEWMA(qpRepo["shortEwmaPeriods"].get<int>());
           qpRepo["_old_longEwmaPeriods"] = qpRepo["longEwmaPeriods"].get<int>(); // setting on start EWMA periods to _old_ place holders incase change during bot active
           qpRepo["_old_mediumEwmaPeriods"] = qpRepo["mediumEwmaPeriods"].get<int>();
           qpRepo["_old_shortEwmaPeriods"] = qpRepo["shortEwmaPeriods"].get<int>();
         */
        if(qpRepo["longEwmaPeriods"].get<int>() != qpRepo["_old_longEwmaPeriods"].get<int>() )
        {
                mgEwmaL = LoadEWMA(qpRepo["longEwmaPeriods"].get<int>());
                qpRepo["_old_longEwmaPeriods"] = qpRepo["longEwmaPeriods"].get<int>();
        } else { calcEwma(&mgEwmaL, qpRepo["longEwmaPeriods"].get<int>()); }
        if(qpRepo["mediumEwmaPeriods"].get<int>() != qpRepo["_old_mediumEwmaPeriods"].get<int>() )
        {
                mgEwmaM = LoadEWMA(qpRepo["mediumEwmaPeriods"].get<int>());
                qpRepo["_old_mediumEwmaPeriods"] = qpRepo["mediumEwmaPeriods"].get<int>();
        } else { calcEwma(&mgEwmaM, qpRepo["mediumEwmaPeriods"].get<int>()); }
        if(qpRepo["shortEwmaPeriods"].get<int>() != qpRepo["_old_shortEwmaPeriods"].get<int>() )
        {
                mgEwmaS = LoadEWMA(qpRepo["shortEwmaPeriods"].get<int>());
                qpRepo["_old_shortEwmaPeriods"] = qpRepo["shortEwmaPeriods"].get<int>();
        } else { calcEwma(&mgEwmaS, qpRepo["shortEwmaPeriods"].get<int>()); }
        if(qpRepo["take_profit_active"].get<bool>() && ( qpRepo["ewmaProfit"].get<int>() != qpRepo["OldewmaProfit"].get<int>() ) ) {
                cout << FN::uiT()  << "ewma7\n";
                mgEwmaProfit = LoadEWMA(qpRepo["ewmaProfit"].get<int>());
                qpRepo["OldewmaProfit"] = qpRepo["ewmaProfit"].get<int>();
        } else if(qpRepo["take_profit_active"].get<bool>() && !qpRepo["take_profit_active_old"].get<bool>() ) {
                cout << FN::uiT()  << "ewma5\n";
                qpRepo["take_profit_active_old"] = true;
                mgEwmaProfit = LoadEWMA(qpRepo["ewmaProfit"].get<int>());
        } else if(!qpRepo["take_profit_active"].get<bool>() && qpRepo["take_profit_active_old"].get<bool>() ) {
                cout << FN::uiT()  << "ewma4\n";
                qpRepo["take_profit_active_old"] = false;
        } else if(qpRepo["take_profit_active"].get<bool>() ) {
                cout << "normally setting ewmaProfit!\n";
                calcEwma(&mgEwmaProfit, qpRepo["ewmaProfit"].get<int>());
        } else {
                cout << FN::uiT()  << "ewma last resort?\n";
                calcEwma(&mgEwmaProfit, qpRepo["ewmaProfit"].get<int>());
        }



        //calcASP();
        //calcSafety();
        calcTargetPos();
        calcASP();
        calcSafety();
        EV::evUp("PositionBroker");
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
        calcEwma(&mgEwmaP, qpRepo["quotingEwmaProtectionPeriods"].get<int>());
        EV::evUp("EWMAProtectionCalculator");
};
static void filter() {
        mGWmktFilter = mGWmkt;
        if (empty()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
                filter(mSide::Bid == (mSide)it->second["side"].get<int>() ? "bids" : "asks", it->second);
        if (!empty()) {
                fairV();
                EV::evUp("FilteredMarket");
        }
};
static void filter(string k, json o) {
        for (json::iterator it = mGWmktFilter[k].begin(); it != mGWmktFilter[k].end(); )
                if (abs((*it)["price"].get<double>() - o["price"].get<double>()) < gw->minTick) {
                        (*it)["size"] = (*it)["size"].get<double>() - o["quantity"].get<double>();
                        if ((*it)["size"].get<double>() < gw->minTick) mGWmktFilter[k].erase(it);
                        break;
                } else ++it;
};
static void fairV() {
        if (empty()) return;
        double mgFairValue_ = mgFairValue;
        mgFairValue = FN::roundNearest(
                mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
                ? (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>()) / 2
                : (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() * mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>() * mGWmktFilter["/bids/0/size"_json_pointer].get<double>()) / (mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/size"_json_pointer].get<double>()),
                gw->minTick
                );
        if (!mgFairValue or (mgFairValue_ and abs(mgFairValue - mgFairValue_) < gw->minTick)) return;
        EV::evUp("FairValue");
        UI::uiSend(uiTXT::FairValue, {{"price", mgFairValue}}, true);
};
static void cleanStdev() {
        size_t periods = (size_t)qpRepo["quotingStdevProtectionPeriods"].get<int>();
        if (mgStatFV.size()>periods) mgStatFV.erase(mgStatFV.begin(), mgStatFV.end()-periods);
        if (mgStatBid.size()>periods) mgStatBid.erase(mgStatBid.begin(), mgStatBid.end()-periods);
        if (mgStatAsk.size()>periods) mgStatAsk.erase(mgStatAsk.begin(), mgStatAsk.end()-periods);
        if (mgStatTop.size()>periods*2) mgStatTop.erase(mgStatTop.begin(), mgStatTop.end()-(periods*2));
};
static void calcStdev() {
        cleanStdev();
        if (mgStatFV.size() < 2 or mgStatBid.size() < 2 or mgStatAsk.size() < 2 or mgStatTop.size() < 4) return;
        double k = qpRepo["quotingStdevProtectionFactor"].get<double>();
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
        mgSMA3.push_back(mgFairValue);

        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
                SMA3 += *it;
        SMA3 /= mgSMA3.size();
        mgSMA3G = SMA3;
        double newTargetPosition = 0;
        if(qpRepo["take_profit_active"].get<bool>()) {
                /*If ewma take profit > SMA3
                        newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"]
                        else
                        If ewma take profit < SMA3
                        new TargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"] - Take_profit_calc.
                        */
                cout << FN::uiT()  << "mgEwmaProfit: " << mgEwmaProfit << " SMA3: " << SMA3 << "\n";
                double takeProfit = (((qpRepo["take_profic_percent"].get<double>()/100) * 2) / 100) - 1;
                if(mgEwmaProfit < SMA3) {
                         newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / (qpRepo["ewmaSensiblityPercentage"].get<double>() - takeProfit) );
                         cout << FN::uiT()  << "EWMA Profit  < SMA3 " << mgEwmaProfit << " | " << SMA3  << " Target: " << newTargetPosition <<  "\n";
                         cout << FN::uiT()  << "EWMA Profit Take Profit: " << takeProfit << "\n";
                         qpRepo["takeProfitNow"] = true;
                }
                if(mgEwmaProfit > SMA3) {
                         newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / (qpRepo["ewmaSensiblityPercentage"].get<double>() - takeProfit) );
                         cout << FN::uiT()  << "EWMA Profit  < SMA3 " << mgEwmaProfit << " | " << SMA3  << " Target: " << newTargetPosition <<  "\n";
                         cout << FN::uiT()  << "EWMA Profit Take Profit: " << takeProfit << "\n";
                         qpRepo["takeProfitNow"] = true;
                         newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
                          cout << FN::uiT()  << "EWMA Profit  > SMA3 " << mgEwmaProfit << " | " << SMA3  << " Target: " << newTargetPosition <<  "\n";
                          qpRepo["takeProfitNow"] = false;
                }


        } else {
                if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LMS) {
                        double newTrend = ((SMA3 * 100 / mgEwmaL) - 100);
                        double newEwmacrossing = ((mgEwmaS * 100 / mgEwmaM) - 100);
                        newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
                } else if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LS)
                        newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
                if (newTargetPosition > 1) newTargetPosition = 1;
                else if (newTargetPosition < -1) newTargetPosition = -1;
        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;

        mgTargetPos = newTargetPosition;
};


static void calcASP() {
        qpRepo["aspvalue"] = ((mgEwmaS * 100/ mgEwmaL) - 100);
        cout << FN::uiT()  <<  "ASP Evaluation: " << ((mgEwmaS * 100/ mgEwmaL) - 100) << "\n";
        cout << FN::uiT() <<  "ASP Evaluation: fairV: " << mgFairValue << "\n";
        cout << FN::uiT() <<  "ASP Evaluation: SMA3 Latest: " << mgSMA3G << "\n";
        cout << FN::uiT() << "Current Short: " << mgEwmaS << "\n";
        cout << FN::uiT() << "Current Long: " << mgEwmaL << "\n";
        if(qpRepo["take_profit_active"].get<bool>()) {
                if(mgEwmaProfit < mgSMA3G)
                {
                        qpRepo["asptriggered"] = false;
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
                                                qpRepo["aspvalue"].get<double>() >= qpRepo["asp_high"].get<double>()
                                        )
                                )
                        )
                        ||
                        (
                                (
                                        qpRepo["aspvalue"].get<double>() <= qpRepo["asp_low"].get<double>()
                                )
                        )
                )
                && qpRepo["aspactive"].get<bool>() == true
                ) {
                //  cout << "ASP high?: " << qpRepo["aspvalue"].get<double>() " >= " << qpRepo["asp_high"].get<double>() << " or asp low?: " << qpRepo["aspvalue"].get<double>() << "<= " << qpRepo["asp_low"].get<double>()) << "\n";
                if ( qpRepo["asptriggered"].get<bool>() == false)
                {
                        qpRepo["asptriggered"] = true;
                        cout << FN::uiT() << "ASP has been activated! pDiv should be set to Zero!\n";
                }
                cout << FN::uiT() << "ASP Active! pDiv should be set to Zero!\n";
        } else {
                if(qpRepo["aspactive"].get<bool>() == true && qpRepo["asptriggered"].get<bool>() == true)
                {
                        qpRepo["asptriggered"] = false;
                        cout << FN::uiT() << "ASP Deactivated" << "\n";
                }
        }


}
static void calcSafety() {
        //  unsigned long int SMA33STARTTIME = std::time(nullptr); // get the time since EWMAProtectionCalculator
        mgMATIME.push_back(std::time(nullptr));
        if (mgMATIME.size()>1000) mgMATIME.erase(mgMATIME.begin(), mgMATIME.end()-1);
        // lets make a SMA logging average
        mgWSMA33.push_back(mgSMA3G);
        cout << FN::uiT() << "Safety Active: " << qpRepo["safetyactive"].get<bool>() << "\n";
        cout << FN::uiT() << "Safety Enabled:" << qpRepo["safetynet"].get<bool>() << "\n";

        cout << FN::uiT() << "Safety SMA3 Array size: " << mgWSMA33.size() << "\n";
        if (mgWSMA33.size()>1000) mgWSMA33.erase(mgWSMA33.begin(), mgWSMA33.end()-1);

        if (qpRepo["safetynet"].get<bool>() == false) {
                qpRepo["safemode"]  = (int)mSafeMode::unknown;
                qpRepo["safetyactive"] = false;
                return;
        }


        if((signed)mgWSMA33.size() > qpRepo["safetytime"].get<signed int>()) {
                cout << FN::uiT() << "Entering Safety Check, SMA3Log Array Size: " << mgWSMA33.size()  << " and safetme index is: " << qpRepo["safetytime"].get<int>() << "\n";
                cout << FN::uiT() << "Latest SMA3 Average in deck " << mgWSMA33.back() << " Target SMA3 " << qpRepo["safetytime"].get<int>() << "  Indexes BEHIND is " << mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) << "\n";
                cout << FN::uiT() << "Latest SMA3 TIME in deck " << mgMATIME.back() << " Target SMA3 TIME " << qpRepo["safetytime"].get<int>() << "  Indexes BEHIND is " << mgMATIME.at(mgMATIME.size() - (qpRepo["safetytime"].get<int>()+1)) << "\n";

                if (
                        (
                                (
                                        (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) ) - 100
                                )
                                >
                                (
                                        qpRepo["safetyP"].get<double>()/100
                                )
                        )
                        &&  !qpRepo["safetyactive"].get<bool>()           // make sure we are not already in a safety active state
                        &&  qpRepo["safetynet"].get<bool>()           // make sure safey checkbox is active on UI

                        )
                {
                        //  printf("debug12\n");
                        // activate Safety, Safety buySize
                        double SafeBuyValuation =   (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) - 100);
                        qpRepo["safemode"] = (int)mSafeMode::buy;
                        cout << FN::uiT() << "Setting SafeMode: "<< qpRepo["safemode"].get<int>() << " ENUM Value: " << (int)mSafeMode::buy << "\n";
                        qpRepo["safetyactive"] = true;
                        qpRepo["safetimestart"] = std::time(nullptr);
                        qpRepo["safetyduration"] = qpRepo["safetimestart"].get<int>() + (qpRepo["safetimeOver"].get<int>() * 60000);
                        cout << FN::uiT() << "Activating Safety BUY Mode First SMA3: " << mgWSMA33.back() << " SMA3[index -" <<  qpRepo["safetytime"].get<int>() << "] Value is: " << mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) << " Equals: " << SafeBuyValuation << " which is More than safety Percent: " << qpRepo["safetyP"].get<double>()/100 << "\n";

                }
/*
                if (     (
                                 (
                                         (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) ) - 100
                                 )
                                 <
                                 -1 * (qpRepo["safetyP"].get<double>()/100)
                                 )
                         &&   !qpRepo["safetyactive"].get<bool>()          // make sure we are not already in a safety active state
                         &&   qpRepo["safetynet"].get<bool>()          // make sure safey checkbox is active on UI

                         )
                {
                        double SafeSellValuation = (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) - 100);
                        qpRepo["safemode"] = (int)mSafeMode::sell;
                        cout << "Setting SafeMode: "<< qpRepo["safemode"].get<int>() << " ENUM Value: " << (int)mSafeMode::sell << "\n";
                        qpRepo["safetyactive"] = true;
                        qpRepo["safetimestart"] = std::time(nullptr);
                        qpRepo["safetyduration"] = qpRepo["safetimestart"].get<int>() + (qpRepo["safetimeOver"].get<int>() * 60000);
                        cout << "Activating Safety SELL Mode First SMA3: " << mgWSMA33.back() << " SMA3[index -" <<  qpRepo["safetytime"].get<int>() <<"] Value is: " << mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) << " Equals: " << SafeSellValuation << " which is less than safety Percent: " << -1 * (qpRepo["safetyP"].get<double>()/100) << "\n";
                        cout << "Safety Duration period is: " << qpRepo["safetyduration"].get<unsigned long int>() << "started at: " << qpRepo["safetimestart"].get<unsigned long int>() << " \n";
                }
                */
        }
        if(qpRepo["safetyactive"].get<bool>() == true and qpRepo["safetynet"].get<bool>() == true)
        {
                if (qpRepo["safemode"].get<int>() == (int)mSafeMode::sell) {
                        cout << FN::uiT() << "SAFETY! Safe Mode Selling! " << "\n";
                }
                if(qpRepo["safemode"].get<int>() == (int)mSafeMode::buy) {
                        cout << FN::uiT() << "SAFETY! Safe Mode Buying! " << "\n";
                }
                if(qpRepo["safemode"].get<int>() == (int)mSafeMode::unknown) {
                        cout << FN::uiT() << "SAFETY! Safe Mode unknown!! " << "\n";
                }
                cout << FN::uiT() << "SAFETY! " << " pDiv should now be set to ZERO.\n";
                cout << FN::uiT() << "SAFETY! " << qpRepo["safemode"].get<int>() << "\n";

        }
        // check for safety time is over
        if((signed)mgWSMA33.size() > qpRepo["safetytime"].get<signed int>() ) {
                cout << FN::uiT() << "Entering Safety Check EXIT, SMA3Log Array Size: " << mgWSMA33.size()  << " and safetme index is: " << qpRepo["safetytime"].get<int>() << "\n";

                if(qpRepo["safetyactive"].get<bool>() and qpRepo["safetynet"].get<bool>())          // Check to make sure we ae currently in active safety state and safety box in UI is active
                {

                        cout << FN::uiT() << "is Timer Over?:" << "\n";
                        cout << FN::uiT() << "Current Value: " << mgWSMA33.back() << "\n";
                        cout << FN::uiT() << "Index back: " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << "\n";
                        cout << FN::uiT() << "time back at time index: " << mgMATIME.at(mgMATIME.size() - (qpRepo["safetytime"].get<int>()+1)) << "\n";
                        cout << FN::uiT() << "Current SMA3 time: " << mgMATIME.back() << "\n";
                        cout << FN::uiT() << "time counter: " <<   qpRepo["safetimestart"] << "\n";
                        cout << FN::uiT() << "time Difference: " << difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>())) << "\n";
                        cout << FN::uiT() << "Duration: " << (qpRepo["safetimeOver"].get<int>() * 60) << "\n";
                        // exit sell mode
                        if(     (mgWSMA33.back() > mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) )
                                && (mSafeMode)qpRepo["safemode"].get<int>() == mSafeMode::sell
                                ) {
                                cout<< FN::uiT()  << "SAFETY: Breaking Safety Mode\n";
                                qpRepo["safemode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = false;
                                cout << FN::uiT() << "SAFETY: Exit Sell Mode\n";

                        }
                        if(     (mgWSMA33.back() < mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) )
                                && (mSafeMode)qpRepo["safemode"].get<int>() == mSafeMode::buy
                                ) {
                                cout << FN::uiT() << "SAFETY: Breaking Safety Mode\n";
                                qpRepo["safemode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = false;
                                cout << FN::uiT() <<  "SAFETY: Exit Buy Mode\n";

                        }
                        //  printf("debugzz\n");
                        //  double spacer = mGWSMA33.at(mGWSMA33.size() - qpRepo["safetytime"].get<double>()).get<double>();
                        //if( (mGWSMA33.back() > mGWSMA33.at(mGWSMA33.size() - qpRepo["safetytime"].get<int>()) ) && (qpRepo["safetyduration"].get<unsigned long int>() >= (qpRepo["safetimeOver"].get<unsigned long int>() * 60000)))
                        /*if(



                                difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>()))

                                >=

                                (qpRepo["safetyduration"].get<double>() * 60)





                                )
                           {
                                cout << "Breaking Safey SELL Mode Time over:" << (qpRepo["safetimeOver"].get<int>() * 60) << " was greater than " << difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>())) << "\n";
                                cout << "Breaking Safey SELL Mode: Latest SMA3 Value: " << mgWSMA33.back() << " was Greater than " << mgWSMA33.at(mgWSMA33.size() - (qpRepo["safetytime"].get<int>()+1)) << "\n";
                                qpRepo["safemode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = false;
                                cout << "Exiting safety mode sell\n";
                           }
                         */
                }
        }
        // Set newTargetPosition

        if( qpRepo["safetyactive"].get<bool>() && qpRepo["safetynet"].get<bool>() && (mSafeMode)qpRepo["safemode"].get<int>() == mSafeMode::buy)
        {
                mgTargetPos = 1;
                cout << "newTargetPosition activated to: " << mgTargetPos << "via Safety buy Action\n";
        } else if( qpRepo["safetyactive"].get<bool>() && qpRepo["safetynet"].get<bool>() && (mSafeMode)qpRepo["safemode"].get<int>() == mSafeMode::sell )
        {
                mgTargetPos = -1;
                cout << "newTargetPosition activated to: " << mgTargetPos << "via Safety sell Action\n";
        }




}

static double LoadEWMA(int periods) {
        //  string baseurl = "https://api.cryptowat.ch/markets/bitfinex/ltcusd/ohlc?periods=60";
        cout << FN::uiT()  << "Starting EWMA\n";
        string baseurl = "http://34.227.139.87/MarketPublish/";
        string pair =  CF::cfString("TradedPair");
        pair.erase(std::remove(pair.begin(), pair.end(), '/'), pair.end());
        cout << FN::uiT()  << "pair: " << pair << "\n";
        string exchange =  CF::cfString("EXCHANGE");
        int CurrentTime = std::time(nullptr);
        int doublePeriods = periods * 6;
        int BackTraceStart = CurrentTime - (periods * 60000);
        std::vector<double> EWMAArray;
        double myEWMA = 0;
        double previous = 0;
        bool first = true;
        string fullURL = string(baseurl.append("?periods=").append(std::to_string(doublePeriods)).append("&exchange=").append(exchange).append("&pair=").append(pair));
        cout << FN::uiT()  << "Full URL: " << fullURL << "\n";
        json EWMA = FN::wJet(fullURL);
        cout << FN::uiT()  << "made it past the curl\n";
        //cout << EWMA << "\n";
        for (auto it = EWMA["result"][std::to_string(doublePeriods)].begin(); it != EWMA["result"][std::to_string(doublePeriods)].end(); ++it)
        {

                json EMAArray = it.value();
                cout << "time: " << EMAArray[0].get<int>() << " Value : " << EMAArray[1].get<double>() << "\n";
                myEWMA = MycalcEwma(EMAArray[1].get<double>(), previous,periods);
                previous = myEWMA;


        }
        cout << FN::uiT()  << "period: " << periods << " EWMA is: " << myEWMA << "\n";
        return myEWMA;

}


static double  MycalcEwma(double k, double previous, int periods) {
        //        cout << " pre-K: " << k << "\n";
        //        cout << " previous : " << previous << "\n";

        if (k) {
                double alpha = (double)2 / (periods + 1);
                //cout << "Alpha: " << alpha << "\n";
                k = alpha * previous + (1 - alpha) * k;
                //cout << " post K: " << k << "\n";
        } else k = previous;

        return k;
};
};
}

#endif
