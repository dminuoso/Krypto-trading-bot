namespace K {
class MGtater : public MG {


static void calcASP() {
        qpRepo["aspvalue"] = ((mgEwmaS * 100/ mgEwmaL) - 100);
        cout <<  "ASP Evaluation: " << ((mgEwmaS * 100/ mgEwmaL) - 100) << "\n";
        cout <<  "ASP Evaluation result: " << qpRepo["aspvalue"].get<double>() << "\n";
        cout <<  "ASP Evaluation: fairV: " << mgFairValue << "\n";
        cout <<  "ASP Evaluation: SMA3 Latest: " << mgSMA3G << "\n";
        cout << "Current Short: " << mgEwmaS << "\n";
        cout << "Current Long: " << mgEwmaL << "\n";
        if (
                (
                        (
                                (
                                        mgEwmaS > mgSMA3G
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
                        cout << "ASP has been activated! pDiv should be set to Zero!\n";
                }
                cout << "ASP Active! pDiv should be set to Zero!\n";
        } else {
                if(qpRepo["aspactive"].get<bool>() == true && qpRepo["asptriggered"].get<bool>() == true)
                {
                        qpRepo["asptriggered"] = false;
                        cout << "ASP Deactivated" << "\n";
                }
        }


}
static void calcSafety() {
        //  unsigned long int SMA33STARTTIME = std::time(nullptr); // get the time since EWMAProtectionCalculator
        if (qpRepo["safetynet"].get<bool>() == false) {
                qpRepo["safemode"]  = (int)mSafeMode::unknown;
                qpRepo["safetyactive"] = false;

                return;
        }
        mgMATIME.push_back(std::time(nullptr));
        if (mgMATIME.size()>100) mgMATIME.erase(mgMATIME.begin(), mgMATIME.end()-1);
        // lets make a SMA logging average
        mgWSMA33.push_back(mgSMA3.back());
        if (mgWSMA33.size()>100) mgWSMA33.erase(mgWSMA33.begin(), mgWSMA33.end()-1);
        // Safety time Active Start Checking
        cout << "Safety Active: " << qpRepo["safetyactive"].get<bool>() << "\n";
        cout << "Safety Enabled:" << qpRepo["safetynet"].get<bool>() << "\n";
        if (qpRepo["safemode"].get<int>() == (int)mSafeMode::sell) {
                cout << "SAFETY! Safe Mode Selling! " << "\n";
        }
        if(qpRepo["safemode"].get<int>() == (int)mSafeMode::buy) {
                cout << "SAFETY! Safe Mode Buying! " << "\n";
        }
        if(qpRepo["safemode"].get<int>() == (int)mSafeMode::unknown) {
                cout << "SAFETY! Safe Mode unknown!! " << "\n";
        }
        cout << "SAFETY! " << qpRepo["safemode"].get<signed int>() << "\n";
        if((signed)mgWSMA33.size() > qpRepo["safetytime"].get<signed int>()) {
                cout << "Entering Safety Check, SMA3Log Array Size: " << mgWSMA33.size()  << " and safetme index is: " << qpRepo["safetytime"].get<int>() << "\n";
                cout << "Latest SMA3 Average in deck " << mgWSMA33.back() << " Target SMA3 " << qpRepo["safetytime"].get<int>() << "  Indexes BEHIND is " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << "\n";
                cout << "Latest SMA3 TIME in deck " << mgMATIME.back() << " Target SMA3 TIME " << qpRepo["safetytime"].get<int>() << "  Indexes BEHIND is " << mgMATIME.at(mgMATIME.size() - qpRepo["safetytime"].get<int>()) << "\n";
                cout << "index now - dec Difference: " << difftime((qpRepo["safetimestart"].get<double>()),std::time(nullptr)) << "\n";
                if (
                        (
                                (
                                        (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) ) - 100
                                ) >  (qpRepo["safetyP"].get<double>()/100)
                        )
                        &&  qpRepo["safetyactive"].get<bool>() == false         // make sure we are not already in a safety active state
                        &&  qpRepo["safetynet"].get<bool>() == true         // make sure safey checkbox is active on UI
                        &&  (qpRepo["safemode"].get<int>() == (int)mSafeMode::sell || qpRepo["safemode"].get<int>() == (int)mSafeMode::unknown)
                        )
                {
                        //  printf("debug12\n");
                        // activate Safety, Safety buySize
                        double SafeBuyValuation =   (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) - 100);
                        qpRepo["safemode"] = (int)mSafeMode::buy;
                        cout << "Setting SafeMode: "<< qpRepo["safemode"].get<int>() << " ENUM Value: " << (int)mSafeMode::buy << "\n";
                        qpRepo["safetyactive"] = true;
                        qpRepo["safetimestart"] = std::time(nullptr);
                        qpRepo["safetyduration"] = qpRepo["safetimestart"].get<int>() + (qpRepo["safetimeOver"].get<int>() * 60000);
                        cout << "Activating Safety BUY Mode First SMA3: " << mgWSMA33.back() << " SMA3[index -" <<  qpRepo["safetytime"].get<int>() << "] Value is: " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << " Equals: " << SafeBuyValuation << " which is More than safety Percent: " << (qpRepo["safetyP"].get<double>()/100) << "\n";
                        cout << "Safety Duration period is: " << qpRepo["safetyduration"].get<unsigned long int>() << "started at: " << qpRepo["safetimestart"].get<unsigned long int>() << " \n";
                }

                if (     (
                                 ((mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) ) - 100 ) <  -(qpRepo["safetyP"].get<double>()/100)
                                 )
                         &&   qpRepo["safetyactive"].get<bool>() == false        // make sure we are not already in a safety active state
                         &&   qpRepo["safetynet"].get<bool>() == true        // make sure safey checkbox is active on UI
                         &&  (qpRepo["safemode"].get<int>() == (int)mSafeMode::buy || qpRepo["safemode"].get<int>() == (int)mSafeMode::unknown)
                         )
                {
                        double SafeSellValuation = (mgWSMA33.back() * 100 /  mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) - 100);
                        qpRepo["safemode"] = (int)mSafeMode::sell;
                        cout << "Setting SafeMode: "<< qpRepo["safemode"].get<int>() << " ENUM Value: " << (int)mSafeMode::sell << "\n";
                        qpRepo["safetyactive"] = true;
                        qpRepo["safetimestart"] = std::time(nullptr);
                        qpRepo["safetyduration"] = qpRepo["safetimestart"].get<int>() + (qpRepo["safetimeOver"].get<int>() * 60000);
                        cout << "Activating Safety SELL Mode First SMA3: " << mgWSMA33.back() << " SMA3[index -" <<  qpRepo["safetytime"].get<int>() <<"] Value is: " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << " Equals: " << SafeSellValuation << " which is less than safety Percent: " << -(qpRepo["safetyP"].get<double>()/100) << "\n";
                        cout << "Safety Duration period is: " << qpRepo["safetyduration"].get<unsigned long int>() << "started at: " << qpRepo["safetimestart"].get<unsigned long int>() << " \n";
                }
        }
        if(qpRepo["safetyactive"].get<bool>() == true and qpRepo["safetynet"].get<bool>() == true)
        {
                if (qpRepo["safemode"].get<int>() == (int)mSafeMode::sell) {
                        cout << "SAFETY! Safe Mode Selling! " << "\n";
                }
                if(qpRepo["safemode"].get<int>() == (int)mSafeMode::buy) {
                        cout << "SAFETY! Safe Mode Buying! " << "\n";
                }
                if(qpRepo["safemode"].get<int>() == (int)mSafeMode::unknown) {
                        cout << "SAFETY! Safe Mode unknown!! " << "\n";
                }
                cout << "SAFETY! " << " pDiv should now be set to ZERO.\n";
                cout << "SAFETY! " << qpRepo["safemode"].get<int>() << "\n";

        }
        // check for safety time is over
        if((signed)mgWSMA33.size() > qpRepo["safetytime"].get<signed int>() ) {
                cout << "Entering Safety Check EXIT, SMA3Log Array Size: " << mgWSMA33.size()  << " and safetme index is: " << qpRepo["safetytime"].get<int>() << "\n";

                if(qpRepo["safetyactive"].get<bool>() == true and qpRepo["safetynet"].get<bool>() == true)        // Check to make sure we ae currently in active safety state and safety box in UI is active
                {

                        cout << "is Timer Over?:" << "\n";
                        cout << "Current Value: " << mgWSMA33.back() << "\n";
                        cout << "Index back: " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << "\n";
                        cout << "time back at time index: " << mgMATIME.at(mgMATIME.size() - qpRepo["safetytime"].get<int>()) << "\n";
                        cout << "Current SMA3 time: " << mgMATIME.back() << "\n";
                        cout << "time counter: " <<   qpRepo["safetimestart"] << "\n";
                        cout << "time Difference: " << difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>())) << "\n";

                        if(
                                (
                                        mgWSMA33.back() < mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>())
                                )
                                and
                                (
                                        (
                                                difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>()))
                                                //  difftime(mgMATIME.back(),(mgMATIME.at(mgMATIME.size() - qpRepo["safetimestart"].get<int>())))
                                        )
                                        <
                                        (
                                                (qpRepo["safetyduration"].get<double>() * 60)
                                        )
                                )
                                and
                                (
                                        (mSafeMode)qpRepo["safemode"].get<int>() == mSafeMode::buy
                                )
                                )
                        {
                                cout << "Breaking Safey BUY Mode Time over:" << (qpRepo["safetimeOver"].get<int>() * 60000) << " was greater than " << difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>()))  << "\n";
                                cout << "Breaking Safey BUY Mode: Latest SMA3 Value: " << mgWSMA33.back() << " was less than " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << "\n";
                                qpRepo["safemode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = false;
                                cout << "exiting safety mode buy\n";

                        }
                        //  printf("debugzz\n");
                        //  double spacer = mGWSMA33.at(mGWSMA33.size() - qpRepo["safetytime"].get<double>()).get<double>();
                        //if( (mGWSMA33.back() > mGWSMA33.at(mGWSMA33.size() - qpRepo["safetytime"].get<int>()) ) && (qpRepo["safetyduration"].get<unsigned long int>() >= (qpRepo["safetimeOver"].get<unsigned long int>() * 60000)))
                        if(
                                (
                                        mgWSMA33.back() > mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>())
                                )
                                and
                                (
                                        (
                                                difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>()))
                                        )
                                        <
                                        (
                                                (qpRepo["safetyduration"].get<double>() * 60)
                                        )
                                )
                                and
                                (
                                        (mSafeMode)qpRepo["safemode"].get<int>() == mSafeMode::sell
                                )
                                )
                        {
                                cout << "Breaking Safey SELL Mode Time over:" << (qpRepo["safetimeOver"].get<int>() * 60000) << " was greater than " << difftime(std::time(nullptr),(qpRepo["safetimestart"].get<double>())) << "\n";
                                cout << "Breaking Safey SELL Mode: Latest SMA3 Value: " << mgWSMA33.back() << " was Greater than " << mgWSMA33.at(mgWSMA33.size() - qpRepo["safetytime"].get<int>()) << "\n";
                                qpRepo["safemode"] = (int)mSafeMode::unknown;
                                qpRepo["safetyactive"] = false;
                                cout << "Exiting safety mode sell\n";
                        }
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
static void ProfitTest() {
}


};
}
