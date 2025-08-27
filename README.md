# Cyber-Physical Microgrid Co-Simulation

This repository contains the implementation of my individual project on **Cyber-Physical Microgrid Simulation**.  
It integrates **OMNeT++ (communication domain)** with **Python Pandapower (power system domain)** using **ZeroMQ messaging**, and demonstrates both **normal operation** and **cyber-attack scenarios** (e.g., Man-in-the-Middle, Attacker node).  

The goal is to model how cyber-attacks affect microgrid stability and evaluate countermeasures such as anomaly detection and control responses.  

---

---

## ⚡ Features
- **OMNeT++ Simulation**  
  - DER (Distributed Energy Resource) and Load modules  
  - Microgrid Controller logic  
  - Custom Ethernet switch  
  - Attacker and MiTM modules for cyber-attack modelling  

- **Jupyter Notebook (Python)**  
  - Power flow analysis with Pandapower  
  - Communication with OMNeT++ via ZeroMQ  
  - Anomaly detection (Isolation Forest, etc.) on traffic features  

- **Integration**  
  - Real-time control adjustments when loads change  
  - Attack scenarios affecting DER operation  

---

## 🛠️ Requirements

### OMNeT++ Side
- OMNeT++ 6.0+  
- INET Framework 4.5  

### Python Side
- Python 3.9+  
- Install dependencies:
```bash
pip install pandapower pyzmq scikit-learn matplotlib
