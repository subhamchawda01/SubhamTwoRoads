/*
 * index_utils.cpp
 *
 *  Created on: Sep 29, 2015
 *      Author: diwakar
 */

#include "dvctrade/Indicators/index_utils.hpp"

namespace HFSAT {
std::string GetBovespaIndexConstituentFileList(int yyyymmdd) {
  std::string t_filename = std::string(INDEX_CONSTITUENT_LIST_BASE_DIR) + "/stock_list.txt";
  std::string filename = " ";
  // Look for this date file
  std::stringstream st;
  st << std::string(INDEX_CONSTITUENT_LIST_BASE_DIR) << "/stock_list_" << yyyymmdd << ".txt";
  filename = st.str();
  if (HFSAT::FileUtils::ExistsAndReadable(filename)) {
    return filename;
  }
  int this_period = (yyyymmdd % 10000 + 300) / 400;  // 1, 2, 3
  // for 1
  int st_date = (yyyymmdd / 10000) * 10000 + 100 + 01;
  int end_date = (yyyymmdd / 10000) * 10000 + 400 + 30;
  if (this_period == 2) {
    st_date = (yyyymmdd / 10000) * 10000 + 500 + 01;
    end_date = (yyyymmdd / 10000) * 10000 + 800 + 31;
  } else if (this_period == 3) {
    st_date = (yyyymmdd / 10000) * 10000 + 900 + 01;
    end_date = (yyyymmdd / 10000) * 10000 + 1200 + 31;
  }
  st.str("");
  int t_date = yyyymmdd;
  while (t_date >= st_date) {
    t_date = HFSAT::DateTime::CalcPrevWeekDay(t_date);
    st << std::string(INDEX_CONSTITUENT_LIST_BASE_DIR) << "/stock_list_" << t_date << ".txt";
    filename = st.str();
    if (HFSAT::FileUtils::ExistsAndReadable(filename)) {
      return filename;
    }
    st.str("");
  }

  t_date = yyyymmdd;
  while (t_date <= end_date) {
    t_date = HFSAT::DateTime::CalcNextDay(t_date);
    st << std::string(INDEX_CONSTITUENT_LIST_BASE_DIR) << "/stock_list_" << t_date << ".txt";
    filename = st.str();
    if (HFSAT::FileUtils::ExistsAndReadable(filename)) {
      return filename;
    }
    st.str("");
  }

  return t_filename;
}

void GetFractionInIbovespaIndex(int yyyymmdd, std::vector<double> &contituent_fraction) {
  std::string constlist_file = GetBovespaIndexConstituentFileList(yyyymmdd);
  std::ifstream shortcode_list_file;
  shortcode_list_file.open(constlist_file.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode theoretical_volume_
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens = st_.GetTokens();
      if (tokens.size() < 2) {
        continue;
      }
      if (tokens.size() > 4 && std::string(tokens[1]).compare("Reductor") == 0) {
        continue;
      }
      double frac = atof(tokens[2]);
      contituent_fraction.push_back(frac);
    }
  }
}

void GetTheoreticalVolumeInIbovespaIndex(int yyyymmdd, std::vector<uint64_t> &contituent_fraction) {
  std::string constlist_file = GetBovespaIndexConstituentFileList(yyyymmdd);
  std::ifstream shortcode_list_file;
  shortcode_list_file.open(constlist_file.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode theoretical_volume_
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens = st_.GetTokens();
      if (tokens.size() < 2) {
        continue;
      }
      if (tokens.size() > 4 && std::string(tokens[1]).compare("Reductor") == 0) {
        continue;
      }
      double frac = atoll(tokens[2]);
      contituent_fraction.push_back(frac);
    }
  }
}

void GetConstituentListInIbovespaIndex(int yyyymmdd, std::vector<std::string> &const_list) {
  std::string constlist_file = GetBovespaIndexConstituentFileList(yyyymmdd);
  std::ifstream shortcode_list_file;
  shortcode_list_file.open(constlist_file.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode theoretical_volume_
      PerishableStringTokenizer st_(readline_buffer_, kL1AvgBufferLen);
      const std::vector<const char *> &tokens = st_.GetTokens();
      if (tokens.size() < 2) {
        continue;
      }
      if (tokens.size() > 4 && std::string(tokens[1]).compare("Reductor") == 0) {
        continue;
      }
      std::string shc = std::string(tokens[0]);
      const_list.push_back(shc);
    }
  }
}

std::string GetSectorForBMFStock(std::string stock, int fraction_portfolio) {
  std::string sector = "NSEFINANCEF";
  if (fraction_portfolio == 0) {
    if (stock.compare("EMBR3") == 0) {
      sector = "BMFFEQCgTe";
    }
    if (stock.compare("POMO4") == 0) {
      sector = "BMFFEQCgTe";
    }
    if (stock.compare("BRFS3") == 0) {
      sector = "BMFEQCncFp";
    }
    if (stock.compare("CSAN3") == 0) {
      sector = "BMFEQCncFp";
    }
    if (stock.compare("JBSS3") == 0) {
      sector = "BMFEQCncFp";
    }
    if (stock.compare("MRFG3") == 0) {
      sector = "BMFEQCncFp";
    }
    if (stock.compare("BEEF3") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("ABEV3") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("VVAR11") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("PCAR4") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("CRUZ3") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("NATU3") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("HYPE3") == 0) {
      sector = "BMFEQCnc";
    }
    if (stock.compare("CYRE3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("EVEN3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("GFSA3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("RSID3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("MRVE3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("PDGR3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("ALLL3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("RUMO3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("CCRO3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("RLOG3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("ECOR3") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("GOLL4") == 0) {
      sector = "BMFEQConsTr";
    }
    if (stock.compare("LAME4") == 0) {
      sector = "BMFEQRet";
    }
    if (stock.compare("LREN3") == 0) {
      sector = "BMFEQRet";
    }
    if (stock.compare("HGTX3") == 0) {
      sector = "BMFEQRet";
    }
    if (stock.compare("IGTA3") == 0) {
      sector = "BMFEQRet";
    }
    if (stock.compare("QUAL3") == 0) {
      sector = "BMFEQRet";
    }
    if (stock.compare("RADL3") == 0) {
      sector = "BMFEQRet";
    }
    if (stock.compare("MPLU3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("VLID3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("TOTS3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("MDIA3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("ESTC3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("KROT3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("RENT3") == 0) {
      sector = "BMFEQDiv";
    }
    if (stock.compare("BRML3") == 0) {
      sector = "BMFEQRlst";
    }
    if (stock.compare("BRPR3") == 0) {
      sector = "BMFEQRlst";
    }
    if (stock.compare("MULT3") == 0) {
      sector = "BMFEQRlst";
    }
    if (stock.compare("BRAP4") == 0) {
      sector = "BMFEQRlst";
    }
    if (stock.compare("UGPA3") == 0) {
      sector = "BMFEQRlst";
    }
    if (stock.compare("BRSR6") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("BBDC3") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("BBDC4") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("BBAS3") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("ITSA4") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("ITUB4") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("SANB11") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("BVMF3") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("BBSE3") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("SULA11") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("CTIP3") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("CIEL3") == 0) {
      sector = "BMFEQFI";
    }
    if (stock.compare("DTEX3") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("FIBR3") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("KLBN11") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("SUZB5") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("VALE3") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("VALE5") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("BRKM5") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("GGBR4") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("GOAU4") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("CSNA3") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("USIM5") == 0) {
      sector = "BMFEQBM";
    }
    if (stock.compare("PETR4") == 0) {
      sector = "BMFEQOGB";
    }
    if (stock.compare("PETR3") == 0) {
      sector = "BMFEQOGB";
    }
    if (stock.compare("OIBR4") == 0) {
      sector = "BMFEQTlc";
    }
    if (stock.compare("VIVT4") == 0) {
      sector = "BMFEQTlc";
    }
    if (stock.compare("TIMP3") == 0) {
      sector = "BMFEQTlc";
    }
    if (stock.compare("SBSP3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("TAEE11") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("CMIG4") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("CESP6") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("CPLE6") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("CPFE3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("GETI4") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("EQTL3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("WEGE3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("ELET3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("ELET6") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("ELPL4") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("ENBR3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("LIGT3") == 0) {
      sector = "BMFEQUt";
    }
    if (stock.compare("TBLE3") == 0) {
      sector = "BMFEQUt";
    }
  } else {
    if (stock.compare("EMBR3") == 0) {
      sector = "BMFEQFrCgTe";
    }
    if (stock.compare("POMO4") == 0) {
      sector = "BMFEQFrCgTe";
    }
    if (stock.compare("BRFS3") == 0) {
      sector = "BMFEQFrCncFp";
    }
    if (stock.compare("CSAN3") == 0) {
      sector = "BMFEQFrCncFp";
    }
    if (stock.compare("JBSS3") == 0) {
      sector = "BMFEQFrCncFp";
    }
    if (stock.compare("MRFG3") == 0) {
      sector = "BMFEQFrCncFp";
    }
    if (stock.compare("ABEV3") == 0) {
      sector = "BMFEQFrCnc";
    }
    if (stock.compare("PCAR4") == 0) {
      sector = "BMFEQFrCnc";
    }
    if (stock.compare("CRUZ3") == 0) {
      sector = "BMFEQFrCnc";
    }
    if (stock.compare("NATU3") == 0) {
      sector = "BMFEQFrCnc";
    }
    if (stock.compare("HYPE3") == 0) {
      sector = "BMFEQFrCnc";
    }
    if (stock.compare("CYRE3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("EVEN3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("GFSA3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("RSID3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("MRVE3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("PDGR3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("ALLL3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("MYPK3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("CCRO3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("RLOG3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("ECOR3") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("GOLL4") == 0) {
      sector = "BMFEQFrConsTr";
    }
    if (stock.compare("LAME4") == 0) {
      sector = "BMFEQFrRet";
    }
    if (stock.compare("LREN3") == 0) {
      sector = "BMFEQFrRet";
    }
    if (stock.compare("HGTX3") == 0) {
      sector = "BMFEQFrRet";
    }
    if (stock.compare("QUAL3") == 0) {
      sector = "BMFEQFrRet";
    }
    if (stock.compare("ODPV3") == 0) {
      sector = "BMFEQFrRet";
    }
    if (stock.compare("SEER3") == 0) {
      sector = "BMFEQFrDiv";
    }
    if (stock.compare("ESTC3") == 0) {
      sector = "BMFEQFrDiv";
    }
    if (stock.compare("ANIM3") == 0) {
      sector = "BMFEQFrDiv";
    }
    if (stock.compare("KROT3") == 0) {
      sector = "BMFEQFrDiv";
    }
    if (stock.compare("RENT3") == 0) {
      sector = "BMFEQFrDiv";
    }
    if (stock.compare("BRML3") == 0) {
      sector = "BMFEQFrRlst";
    }
    if (stock.compare("BRPR3") == 0) {
      sector = "BMFEQFrRlst";
    }
    if (stock.compare("MULT3") == 0) {
      sector = "BMFEQFrRlst";
    }
    if (stock.compare("BRAP4") == 0) {
      sector = "BMFEQFrRlst";
    }
    if (stock.compare("UGPA3") == 0) {
      sector = "BMFEQFrRlst";
    }
    if (stock.compare("BBTG11") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("BBDC3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("BBDC4") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("BBAS3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("ITSA4") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("ITUB4") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("SANB11") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("PSSA3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("BBSE3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("BBSE3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("CTIP3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("CIEL3") == 0) {
      sector = "BMFEQFrFI";
    }
    if (stock.compare("DTEX3") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("FIBR3") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("KLBN11") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("QGEP3") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("SUZB5") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("VALE3") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("VALE5") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("BRKM5") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("GGBR4") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("GOAU4") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("CSNA3") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("USIM5") == 0) {
      sector = "BMFEQFrBM";
    }
    if (stock.compare("PETR4") == 0) {
      sector = "BMFEQFrOGB";
    }
    if (stock.compare("PETR3") == 0) {
      sector = "BMFEQFrOGB";
    }
    if (stock.compare("OIBR4") == 0) {
      sector = "BMFEQFrTlc";
    }
    if (stock.compare("VIVT4") == 0) {
      sector = "BMFEQFrTlc";
    }
    if (stock.compare("TIMP3") == 0) {
      sector = "BMFEQFrTlc";
    }
    if (stock.compare("SBSP3") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("CMIG4") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("CESP6") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("CPLE6") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("CPFE3") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("ELET3") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("ELET6") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("ELPL4") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("ENBR3") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("LIGT3") == 0) {
      sector = "BMFEQFrUt";
    }
    if (stock.compare("TBLE3") == 0) {
      sector = "BMFEQFrUt";
    }
  }
  return sector;
}

std::string GetConstituentListFileNIFTY(int yyyymmdd, const std::string &portfolio) {
  std::string t_filename = std::string(NSE_INDEX_INFO_DIR) + "/" + portfolio + "_WGHTS_.txt";
  std::string filename = " ";
  // Look for this date file
  std::stringstream st;
  st << std::string(NSE_INDEX_INFO_DIR) << "/" + portfolio + "_WGHTS_" << yyyymmdd << "_.txt";
  filename = st.str();

  if (HFSAT::FileUtils::ExistsAndReadable(filename)) {
    return filename;
  }

  // Changing the Logic to look for any file in past 30 days

  int t_date = yyyymmdd;
  for (int i = 0; i < 30; i++) {
    t_date = HFSAT::DateTime::CalcPrevWeekDay(t_date);
    st << std::string(NSE_INDEX_INFO_DIR) << "/" + portfolio + "_WGHTS_" << t_date << "_.txt";
    filename = st.str();
    if (HFSAT::FileUtils::ExistsAndReadable(filename)) {
      //      std::cout << "Filename is : " << filename << std::endl;
      return filename;
    }
    st.str("");
  }

  // std::cout << "Filename is : " << t_filename << std::endl;
  return t_filename;
}

void GetConstituentListInNIFTY(int yyyymmdd, std::vector<std::string> &const_list, const std::string &portfolio) {
  std::string constlist_file = GetConstituentListFileNIFTY(yyyymmdd, portfolio);
  std::ifstream shortcode_list_file;
  shortcode_list_file.open(constlist_file.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode theoretical_volume_
      char *symbol = strtok(readline_buffer_, " ");
      if (symbol != nullptr) {
        // char *symbol = strtok(nullptr, ",");
        //    std::cout << "Symbol : " << symbol << std::endl;

        if (symbol != nullptr && strncmp(symbol, "SYMBOL", 6) != 0) {
          const_list.push_back(symbol);
          continue;
        }
      }
    }
  }
}

void GetIthColumNIFTY(int yyyymmdd, std::vector<double> &value_vec, int idx, const std::string &portfolio) {
  std::string constlist_file = GetConstituentListFileNIFTY(yyyymmdd, portfolio);
  std::ifstream shortcode_list_file;
  shortcode_list_file.open(constlist_file.c_str(), std::ifstream::in);
  if (shortcode_list_file.is_open()) {
    while (shortcode_list_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      shortcode_list_file.getline(readline_buffer_, kL1AvgBufferLen);
      if (readline_buffer_[0] == '#') {
        continue;
      }
      // # shortcode theoretical_volume_
      char *symbol = strtok(readline_buffer_, " ");
      if (symbol != nullptr) {
        // char *symbol = strtok(nullptr, ",");

        if (strncmp(symbol, "SYMBOL", 6) == 0) {
          continue;
        }

        //     std::cout << "Symbol : " << symbol << " Cid: "<< idx << std::endl;

        int weight_id = idx;
        int current_id = 1;

        while (current_id < weight_id && symbol != nullptr) {
          symbol = strtok(nullptr, " ");
          current_id++;
        }

        //   std::cout << "Symbol : " << symbol << std::endl;

        if (current_id == weight_id && symbol != nullptr) {
          //   std::cout << " Betasym: " << symbol << " cid: " << current_id << " wt: " << weight_id << std::endl;
          value_vec.push_back(atof(symbol));
        } else {
          std::cerr << "malformed line in GetBetaInNIFTY\n";
        }
      }
    }
  }
}

double GetIndexFactorNIFTY(int yyyymmdd, const std::string &portfolio) {
  std::string filename = std::string(NSE_INDEX_INFO_DIR) + "/" + portfolio + "_INDEX_FACTOR.txt";

  double index_factor_ = 1.00;

  std::ifstream index_factor_file;
  index_factor_file.open(filename.c_str(), std::ifstream::in);
  if (index_factor_file.is_open()) {
    while (index_factor_file.good()) {
      const int kL1AvgBufferLen = 1024;
      char readline_buffer_[kL1AvgBufferLen];
      bzero(readline_buffer_, kL1AvgBufferLen);
      index_factor_file.getline(readline_buffer_, kL1AvgBufferLen);
      // # shortcode theoretical_volume
      char *symbol = strtok(readline_buffer_, " ");
      if (symbol != nullptr) {
        // char *symbol = strtok(nullptr, ",");
        if (strncmp(symbol, "DATE", 4) == 0) {
          continue;
        }

        int date_ = atoi(symbol);

        symbol = strtok(nullptr, " ");
        symbol = strtok(nullptr, " ");
        symbol = strtok(nullptr, " ");
        if (date_ < yyyymmdd) {
          index_factor_ = atof(symbol);
          continue;
        }
        break;
      }
    }
  }
  return index_factor_;
}

void GetConstituentFractionInNIFTY(int yyyymmdd, std::vector<double> &constituent_fraction,
                                   const std::string &portname) {
  GetIthColumNIFTY(yyyymmdd, constituent_fraction, 2, portname);
}

void GetConstituentEquityVolNIFTY(int yyyymmdd, std::vector<double> &market_vol, const std::string &portname) {
  GetIthColumNIFTY(yyyymmdd, market_vol, 2, portname);
}
}
