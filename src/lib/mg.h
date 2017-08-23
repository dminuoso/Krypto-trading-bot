#ifndef K_MG_H_
#define K_MG_H_

namespace K {
static vector<mGWmt> mGWmt_;
static json mGWmkt;
static json mGWmktFilter;
static double mGWfairV = 0;
static double mGWEwmaL = 0;
static double mGWEwmaM = 0;
static double mGWEwmaS = 0;
static vector<double> mGWSMA3;
static vector<double> mGWSMA33;    // Logging SMA3 values
static vector<double> mSMATIME;    // logging SMA3 value timestamps
class MG {
public:
static void main(Local<Object> exports) {
        load();
        EV::evOn("MarketTradeGateway", [](json k) {
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
                        });
        EV::evOn("MarketDataGateway", [](json o) {
                                mktUp(o);
                        });
        EV::evOn("GatewayMarketConnect", [](json k) {
                                if ((mConnectivityStatus)k["/0"_json_pointer].get<int>() == mConnectivityStatus::Disconnected)
                                        mktUp({});
                        });
        EV::evOn("QuotingParameters", [](json k) {
                                fairV();
                        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        NODE_SET_METHOD(exports, "mgFilter", MG::_mgFilter);
        NODE_SET_METHOD(exports, "mgFairV", MG::_mgFairV);
        NODE_SET_METHOD(exports, "mgEwmaLong", MG::_mgEwmaLong);
        NODE_SET_METHOD(exports, "mgEwmaMedium", MG::_mgEwmaMedium);
        NODE_SET_METHOD(exports, "mgEwmaShort", MG::_mgEwmaShort);
        NODE_SET_METHOD(exports, "mgTBP", MG::_mgTBP);
};
private:
static void load() {
        json k = DB::load(uiTXT::EWMAChart);
        if (k.size()) {
                if (k["/0/fairValue"_json_pointer].is_number())
                        mGWfairV = k["/0/fairValue"_json_pointer].get<double>();
                if (k["/0/ewmaLong"_json_pointer].is_number())
                        mGWEwmaL = k["/0/ewmaLong"_json_pointer].get<double>();
                if (k["/0/ewmaMedium"_json_pointer].is_number())
                        mGWEwmaM = k["/0/ewmaMedium"_json_pointer].get<double>();
                if (k["/0/ewmaShort"_json_pointer].is_number())
                        mGWEwmaS = k["/0/ewmaShort"_json_pointer].get<double>();
        }
};

static void _mgEwmaLong(const FunctionCallbackInfo<Value>& args ) {
        mGWEwmaL = calcEwma(args[0]->NumberValue(), mGWEwmaL, qpRepo["longEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWEwmaL));
};
static void _mgEwmaMedium(const FunctionCallbackInfo<Value>& args) {
        mGWEwmaM = calcEwma(args[0]->NumberValue(), mGWEwmaM, qpRepo["mediumEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWEwmaM));
};
static void _mgEwmaShort(const FunctionCallbackInfo<Value>& args) {
        mGWEwmaS = calcEwma(args[0]->NumberValue(), mGWEwmaS, qpRepo["shortEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWEwmaS));
};
static void _mgTBP(const FunctionCallbackInfo<Value>& args) {
        mGWSMA3.push_back(args[0]->NumberValue());
        double newLong = args[1]->NumberValue();
        double newMedium = args[2]->NumberValue();
        double newShort = args[3]->NumberValue();
        printf("Short: %f Medium: %f Long: %f \n", newShort, newMedium, newLong);
        if (mGWSMA3.size()>3) mGWSMA3.erase(mGWSMA3.begin(), mGWSMA3.end()-3);
        double SMA3 = 0;
        double SMA33 = 0;
        for (vector<double>::iterator it = mGWSMA3.begin(); it != mGWSMA3.end(); ++it)
                SMA3 += *it;
        SMA3 /= mGWSMA3.size();

        unsigned long int SMA33STARTTIME = std::time(nullptr); // get the time since EWMAProtectionCalculator

        // lets make a SMA logging average
        mGWSMA33.push_back(SMA3);
        if (mGWSMA33.size()>100) mGWSMA33.erase(mGWSMA33.begin(), mGWSMA33.end()-1);
        for (vector<double>::iterator iz = mGWSMA33.begin(); iz != mGWSMA33.end(); ++iz)
                SMA33 += *iz;
        SMA33 /= mGWSMA33.size();


                mSMATIME.push_back((int)SMA33STARTTIME);
        if (mSMATIME.size()>100) mSMATIME.erase(mSMATIME.begin(), mSMATIME.end()-1);


        double newTargetPosition = 0;
        if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LMS) {
                double newTrend = ((SMA3 * 100 / newLong) - 100);
                double newEwmacrossing = ((newShort * 100 / newMedium) - 100);
                newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
                qpRepo["aspvalue"] = newTargetPosition;
        } else if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LS) {
                newTargetPosition = ((newShort * 100/ newLong) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
                qpRepo["aspvalue"] = newTargetPosition;
                printf("ASP: value: %f\n", newTargetPosition);
        }
        newTargetPosition = ((newShort * 100/ newLong) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        printf("ASP: ewma?: %f", qpRepo["ewmaSensiblityPercentage"].get<double>() );
        qpRepo["aspvalue"] = newTargetPosition;
        printf("ASP: value: %f\n", newTargetPosition);
        printf("ASP: value: %f\n", qpRepo["aspvalue"].get<double>());
        printf("ASP: testvalue: %f\n",((newShort * 100/ newLong) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>()));


        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;

        if ( (qpRepo["aspvalue"].get<double>() >= qpRepo["asp_high"].get<double>() || qpRepo["aspvalue"].get<double>() <= qpRepo["asp_low"].get<double>()) && qpRepo["aspactive"].get<bool>() == true ) {
                //  pdiv changes here..

        }
        printf("ASP: ASPVALUE %f  ASPHIGH: %f ASPLOW: %f  ASPACTIVE: %d\n", qpRepo["aspvalue"].get<double>(), qpRepo["asp_high"].get<double>(), qpRepo["asp_low"].get<double>(), qpRepo["aspactive"].get<bool>()  );
        // relocating this for now...  args.GetReturnValue().Set(Number::New(args.GetIsolate(), newTargetPosition));

        // lets do some SMA math to see if we can buy or sell safety time!

      //  printf("SMA33 Buy Mode Active  First Value: %f  Last Value %f safetyPercent: %f \n", mGWSMA33.back(), mGWSMA33.front(), qpRepo["safetyP"].get<double>()/100);
      //  printf("SMA33 Debugging:  Is SafetyActive: %d  Is Safety Even On: %d\n",qpRepo["safetyactive"].get<bool>(),qpRepo["safetynet"].get<bool>()  );


        printf("debug10\n");
        if(mGWSMA33.size() > 3) {
          printf("debug11\n");
                if (  mGWSMA33.back() * 100 / mGWSMA33.front() - 100 >  qpRepo["safetyP"].get<double>()/100  &&  qpRepo["safetyactive"].get<bool>() == false  &&  qpRepo["safetynet"].get<bool>() == true)
                {
                  printf("debug12\n");
                        // activate Safety, Safety buySize
                        qpRepo["mSafeMode"] = (int)mSafeMode::buy;
                        qpRepo["safetyactive"] = 1;
                        qpRepo["safetimestart"] = (unsigned long int)SMA33STARTTIME;
                        printf("SMA33 Buy Mode Active  First Value: %f  Last Value %f safetyPercent: %f \n", mGWSMA33.back(), mGWSMA33.front(), qpRepo["safetyP"].get<double>()/100);
                        printf("SMA33 Start Time started at: %lu \n", qpRepo["safetimestart"].get<unsigned long int>());
                        qpRepo["safetyduration"] = std::time(nullptr) - qpRepo["safetimestart"].get<unsigned long int>();
                }
                if (  mGWSMA33.back() * 100 / mGWSMA33.front() - 100 <  qpRepo["safetyP"].get<double>()/100 &&  qpRepo["safetyactive"].get<bool>() == false &&  qpRepo["safetynet"].get<bool>() == true)
                {
                  printf("debug13\n");
                        qpRepo["mSafeMode"] = (int)mSafeMode::sell;
                        qpRepo["safetimestart"] = (unsigned long int)SMA33STARTTIME;
                        qpRepo["safetyactive"] = 1;
                        printf("SMA33 Sell Mode Active: First Value: %f  Last Value %f safetyPercent: %f \n", mGWSMA33.back(), mGWSMA33.front(),qpRepo["safetyP"].get<double>()/100 );
                        printf("SMA33 Start Time started at: %lu \n", qpRepo["safetimestart"].get<unsigned long int>());
                       qpRepo["safetyduration"] = std::time(nullptr) - qpRepo["safetimestart"].get<unsigned long int>();
                }

        }

        printf("Duration: %lu  Start time: %lu Time Starated: %lu\n", qpRepo["safetyduration"].get<unsigned long int>(), std::time(nullptr), qpRepo["safetimestart"].get<unsigned long int>() );
        if(mGWSMA33.size() > 3  ) {
          printf("Debug1\n");
          printf("SMA33 Safetytime: %f\n", qpRepo["safetytime"].get<int>());
                if(mGWSMA33.size() > qpRepo["safetytime"].get<int>()  )// checking to make sure array size is larger than what we are looking for.. otherwise.. KABOOOM!
                {
                  printf("debug2\n");
                      //  if( (mGWSMA33.back() < mGWSMA33.at(mGWSMA33.size() - qpRepo["safetytime"].get<int>()) ) && (qpRepo["safetyduration"].get<unsigned long int>() >= (qpRepo["safetimeOver"].get<unsigned long int>() * 60000)))
                      printf("arraySize: %lu", mGWSMA33.size() );
                        if( mGWSMA33.back() < mGWSMA33.at(2) )
                        {
                          printf("debug3\n");
                                qpRepo["mSafeMode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = 0;
                                printf("SMA33 Safety Mode is over \n");
                                printf("debug4\n");
                        }
                        if( (mGWSMA33.back() > mGWSMA33.at(mGWSMA33.size() - qpRepo["safetytime"].get<int>()) ) && (qpRepo["safetyduration"].get<unsigned long int>() >= (qpRepo["safetimeOver"].get<unsigned long int>() * 60000)))
                        {
                          printf("debug5\n");
                                qpRepo["mSafeMode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = 0;
                                printf("SMA33 Safety Mode is over \n");
                                printf("debug6\n");
                        }
                }
        }

        if( qpRepo["safetyactive"].get<bool>() && qpRepo["safetynet"].get<bool>() && (mSafeMode)qpRepo["mSafeMode"].get<int>() == mSafeMode::buy  )
        {
                newTargetPosition = 1;
        } else if( qpRepo["safetyactive"].get<bool>() && qpRepo["safetynet"].get<bool>() && (mSafeMode)qpRepo["mSafeMode"].get<int>() == mSafeMode::sell  )
        {
                newTargetPosition = -1;
        }


        args.GetReturnValue().Set(Number::New(args.GetIsolate(), newTargetPosition));






};




static void _mgFilter(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, mGWmktFilter.dump())).ToLocalChecked());
};
static void _mgFairV(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWfairV));
};
static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
                k.push_back(tradeUp(mGWmt_[i]));
        return k;
};
static json onSnapFair(json z) {
        return {{{"price", mGWfairV}}};
};
static void mktUp(json k) {
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
static void filter() {
        mGWmktFilter = mGWmkt;
        if (mGWmktFilter.is_null() or mGWmktFilter["bids"].is_null() or mGWmktFilter["asks"].is_null()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
                filter(mSide::Bid == (mSide)it->second["side"].get<int>() ? "bids" : "asks", it->second);
        if (!mGWmktFilter["bids"].is_null() and !mGWmktFilter["asks"].is_null()) {
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
        if (mGWmktFilter.is_null() or mGWmktFilter["/bids/0"_json_pointer].is_null() or mGWmktFilter["/asks/0"_json_pointer].is_null()) return;
        double mGWfairV_ = mGWfairV;
        mGWfairV = SD::roundNearest(
                mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
                ? (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>()) / 2
                : (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() * mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>() * mGWmktFilter["/bids/0/size"_json_pointer].get<double>()) / (mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/size"_json_pointer].get<double>()),
                gw->minTick
                );
        if (!mGWfairV or (mGWfairV_ and abs(mGWfairV - mGWfairV_) < gw->minTick)) return;
        EV::evUp("FairValue");
        UI::uiSend(uiTXT::FairValue, {{"price", mGWfairV}}, true);
};
static double calcEwma(double newValue, double previous, int periods) {
        if (previous) {
                double alpha = 2 / (periods + 1);
                return alpha * newValue + (1 - alpha) * previous;
        }
        return newValue;
};
};
}

#endif
