/*
 * ============================================================
 *  Simulador de CPU - Ciclo Fetch-Decode-Execute
 *  Sistemas Operativos
 *  Compatible con C++98
 * ============================================================
 *
 *  Jerarquia de clases:
 *
 *    Instruccion  (abstracta)
 *    |-- InstruccionSET
 *    |-- InstruccionLDR
 *    |-- InstruccionADD
 *    |-- InstruccionINC
 *    |-- InstruccionDEC
 *    |-- InstruccionSTR
 *    |-- InstruccionSHW
 *    |-- InstruccionPAUSE
 *    `-- InstruccionEND
 *
 *  Uso:
 *    ./simulador -> ejecuta p1.txt, p2.txt, p3.txt
 *    ./simulador prog.txt  -> ejecuta el archivo indicado
 *
 *  Salida: cada programa genera un archivo .out con el mismo
 *  nombre base (p1.txt -> p1.out, p2.txt -> p2.out, etc.)
 * ============================================================
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

// Constantes
const int TAM_MEMORIA = 64;
const int MAX_INSTR = 256;

int memoria[TAM_MEMORIA];

void reiniciarMemoria() {
  for (int i = 0; i < TAM_MEMORIA; i++)
    memoria[i] = 0;
}

// Registros del procesador
struct CPU {
  int ACC;     // Accumulator Register
  int ICR;     // Instruction Counter Register
  int MAR;     // Memory Address Register
  int MDR;     // Memory Data Register
  bool activo; // Estado de la Unidad de Control (UC)

  CPU() : ACC(0), ICR(0), MAR(0), MDR(0), activo(false) {}

  void reiniciar() {
    ACC = 0;
    ICR = 0;
    MAR = 0;
    MDR = 0;
    activo = false;
  }
};

// Funciones auxiliares
int parsearDireccion(const string &s) {
  if (s.size() > 1 && s[0] == 'D') {
    int idx = atoi(s.c_str() + 1);
    if (idx >= 0 && idx < TAM_MEMORIA)
      return idx;
  }
  return -1;
}

bool esNulo(const string &s) { return s == "NULL" || s.empty(); }

// Clase base abstracta
class Instruccion {
protected:
  string op1, op2, op3, op4;

public:
  Instruccion(const string &a = "NULL", const string &b = "NULL",
              const string &c = "NULL", const string &d = "NULL")
      : op1(a), op2(b), op3(c), op4(d) {}

  virtual ~Instruccion() {}

  // Ejecuta la instruccion.
  // Retorna false unicamente cuando la ejecucion debe detenerse.
  virtual bool ejecutar(CPU &cpu) = 0;

  // Nombre del opcode para trazado
  virtual string getNombre() const = 0;

  void imprimir() const {
    cout << getNombre() << " " << op1 << " " << op2 << " " << op3 << " " << op4;
  }
};

// SET: Almacena un valor inmediato en memoria (sin pasar por la ALU).
// Formato: SET Dx valor NULL NULL
class InstruccionSET : public Instruccion {
public:
  InstruccionSET(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "SET"; }

  bool ejecutar(CPU &cpu) {
    int dir = parsearDireccion(op1);
    if (dir == -1) {
      cerr << "  [ERROR SET] Direccion invalida: " << op1 << endl;
      return true;
    }
    int val = atoi(op2.c_str());
    cpu.MAR = dir;
    cpu.MDR = val;
    memoria[dir] = val;
    return true;
  }
};

// LDR: Carga un valor de memoria al Acumulador.
// Formato: LDR Dx NULL NULL NULL
class InstruccionLDR : public Instruccion {
public:
  InstruccionLDR(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "LDR"; }

  bool ejecutar(CPU &cpu) {
    int dir = parsearDireccion(op1);
    if (dir == -1) {
      cerr << "  [ERROR LDR] Direccion invalida: " << op1 << endl;
      return true;
    }
    cpu.MAR = dir;
    cpu.MDR = memoria[dir];
    cpu.ACC = cpu.MDR;
    return true;
  }
};

// ADD: Suma valores en memoria o acumulador.
// Variantes:
//   ADD Dx NULL NULL NULL  ->  ACC = ACC + mem[Dx]
//   ADD Dx Dy NULL NULL    ->  ACC = mem[Dx] + mem[Dy]
//   ADD Dx Dy Dz  NULL     ->  mem[Dz] = mem[Dx] + mem[Dy]; ACC = resultado
class InstruccionADD : public Instruccion {
public:
  InstruccionADD(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "ADD"; }

  bool ejecutar(CPU &cpu) {
    int dir1 = parsearDireccion(op1);
    if (dir1 == -1) {
      cerr << "  [ERROR ADD] Direccion invalida: " << op1 << endl;
      return true;
    }

    if (esNulo(op2)) {
      // Variante 1: ACC += mem[Dx]
      cpu.MAR = dir1;
      cpu.MDR = memoria[dir1];
      cpu.ACC += cpu.MDR;

    } else {
      int dir2 = parsearDireccion(op2);
      if (dir2 == -1) {
        cerr << "  [ERROR ADD] Direccion invalida: " << op2 << endl;
        return true;
      }

      if (esNulo(op3)) {
        // Variante 2: ACC = mem[Dx] + mem[Dy]
        cpu.ACC = memoria[dir1] + memoria[dir2];
        cpu.MDR = cpu.ACC;

      } else {
        // Variante 3: mem[Dz] = mem[Dx] + mem[Dy]
        int dir3 = parsearDireccion(op3);
        if (dir3 == -1) {
          cerr << "  [ERROR ADD] Direccion invalida: " << op3 << endl;
          return true;
        }
        int resultado = memoria[dir1] + memoria[dir2];
        cpu.MAR = dir3;
        cpu.MDR = resultado;
        memoria[dir3] = resultado;
        cpu.ACC = resultado;
      }
    }
    return true;
  }
};

// INC: Incrementa en 1 el valor en memoria.
// Formato: INC Dx NULL NULL NULL
class InstruccionINC : public Instruccion {
public:
  InstruccionINC(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "INC"; }

  bool ejecutar(CPU &cpu) {
    int dir = parsearDireccion(op1);
    if (dir == -1) {
      cerr << "  [ERROR INC] Direccion invalida: " << op1 << endl;
      return true;
    }
    cpu.MAR = dir;
    memoria[dir]++;
    cpu.MDR = memoria[dir];
    cpu.ACC = cpu.MDR;
    return true;
  }
};

// DEC: Decrementa en 1 el valor en memoria.
// Formato: DEC Dx NULL NULL NULL
class InstruccionDEC : public Instruccion {
public:
  InstruccionDEC(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "DEC"; }

  bool ejecutar(CPU &cpu) {
    int dir = parsearDireccion(op1);
    if (dir == -1) {
      cerr << "  [ERROR DEC] Direccion invalida: " << op1 << endl;
      return true;
    }
    cpu.MAR = dir;
    memoria[dir]--;
    cpu.MDR = memoria[dir];
    cpu.ACC = cpu.MDR;
    return true;
  }
};

// STR: Escribe el Acumulador en memoria.
// Formato: STR Dx NULL NULL NULL
class InstruccionSTR : public Instruccion {
public:
  InstruccionSTR(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "STR"; }

  bool ejecutar(CPU &cpu) {
    int dir = parsearDireccion(op1);
    if (dir == -1) {
      cerr << "  [ERROR STR] Direccion invalida: " << op1 << endl;
      return true;
    }
    cpu.MAR = dir;
    cpu.MDR = cpu.ACC;
    memoria[dir] = cpu.ACC;
    return true;
  }
};

// SHW: Muestra el valor de un registro o posición de memoria.
// Formato: SHW [Dx | ACC | ICR | MAR | MDR | UC] NULL NULL NULL
class InstruccionSHW : public Instruccion {
public:
  InstruccionSHW(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "SHW"; }

  bool ejecutar(CPU &cpu) {
    if (op1 == "ACC")
      cout << "  >> ACC = " << cpu.ACC << endl;
    else if (op1 == "ICR")
      cout << "  >> ICR = " << cpu.ICR << endl;
    else if (op1 == "MAR")
      cout << "  >> MAR = " << cpu.MAR << endl;
    else if (op1 == "MDR")
      cout << "  >> MDR = " << cpu.MDR << endl;
    else if (op1 == "UC") {
      cout << "  >> UC  = " << (cpu.activo ? "ACTIVA" : "INACTIVA") << endl;
    } else {
      int dir = parsearDireccion(op1);
      if (dir == -1) {
        cerr << "  [ERROR SHW] Operando invalido: " << op1 << endl;
        return true;
      }
      cout << "  >> " << op1 << " = " << memoria[dir] << endl;
    }
    return true;
  }
};

// PAUSE: Detiene el ciclo para inspección interactiva.
// Formato: PAUSE NULL NULL NULL NULL
class InstruccionPAUSE : public Instruccion {
public:
  InstruccionPAUSE(const string &a, const string &b, const string &c,
                   const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "PAUSE"; }

  bool ejecutar(CPU &cpu) {
    cout << "  [PAUSE] Estado actual del procesador:" << endl;
    cout << "    ACC=" << cpu.ACC << "  ICR=" << cpu.ICR << "  MAR=" << cpu.MAR
         << "  MDR=" << cpu.MDR << endl;
    // cerr: el prompt interactivo debe verse en la terminal
    // aunque cout este redirigido al archivo .out
    cerr << "  [PAUSE] Presione ENTER para continuar...";
    cin.get();
    return true;
  }
};

// END: Finaliza la ejecución del programa.
// Formato: END NULL NULL NULL
class InstruccionEND : public Instruccion {
public:
  InstruccionEND(const string &a, const string &b, const string &c,
                 const string &d)
      : Instruccion(a, b, c, d) {}

  string getNombre() const { return "END"; }

  bool ejecutar(CPU &cpu) {
    cpu.activo = false;
    cout << "  [END] Programa finalizado." << endl;
    return false; // señal de parada para el ciclo de ejecución
  }
};

// Fábrica de instrucciones: Lee texto y construye el objeto correspondiente.
Instruccion *crearInstruccion(const string &linea) {
  istringstream iss(linea);
  string opcode;
  string ops[4];

  // Inicializar operandos como NULL
  for (int i = 0; i < 4; i++)
    ops[i] = "NULL";

  if (!(iss >> opcode) || opcode.empty() || opcode[0] == '#')
    return NULL;

  // Leer hasta 4 operandos
  int k = 0;
  while (k < 4 && (iss >> ops[k]))
    k++;

  if (opcode == "SET")
    return new InstruccionSET(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "LDR")
    return new InstruccionLDR(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "ADD")
    return new InstruccionADD(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "INC")
    return new InstruccionINC(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "DEC")
    return new InstruccionDEC(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "STR")
    return new InstruccionSTR(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "SHW")
    return new InstruccionSHW(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "PAUSE")
    return new InstruccionPAUSE(ops[0], ops[1], ops[2], ops[3]);
  if (opcode == "END")
    return new InstruccionEND(ops[0], ops[1], ops[2], ops[3]);

  cerr << "[AVISO] Opcode desconocido ignorado: " << opcode << endl;
  return NULL;
}

// Generador de nombre de archivo de salida (reemplaza extensión por .out).
string generarNombreOut(const string &nombreArchivo) {
  size_t pos = nombreArchivo.rfind('.');
  if (pos != string::npos)
    return nombreArchivo.substr(0, pos) + ".out";
  return nombreArchivo + ".out";
}

// Clase Simulador: Gestiona la carga y el ciclo Fetch-Decode-Execute.
class Simulador {
private:
  Instruccion *instrucciones[MAX_INSTR];
  int numInstr;
  CPU cpu;

  void liberarInstrucciones() {
    for (int i = 0; i < numInstr; i++) {
      delete instrucciones[i];
      instrucciones[i] = NULL;
    }
    numInstr = 0;
  }

public:
  Simulador() : numInstr(0) {
    for (int i = 0; i < MAX_INSTR; i++)
      instrucciones[i] = NULL;
  }

  ~Simulador() { liberarInstrucciones(); }

  // Carga: Lee el archivo y construye los objetos Instruccion.
  // Lee el archivo de texto, construye los objetos Instruccion
  // y los almacena en el arreglo de instrucciones.
  bool cargar(const char *archivo) {
    liberarInstrucciones();
    cpu.reiniciar();
    reiniciarMemoria();

    ifstream f(archivo);
    if (!f.is_open()) {
      cerr << "[ERROR] No se pudo abrir: " << archivo << endl;
      return false;
    }

    string linea;
    while (getline(f, linea)) {
      // Eliminar blancos iniciales y saltos de linea Windows (\r)
      size_t inicio = linea.find_first_not_of(" \t\r\n");
      if (inicio == string::npos)
        continue;
      if (linea[inicio] == '#')
        continue;
      linea = linea.substr(inicio);

      if (numInstr >= MAX_INSTR) {
        cerr << "[AVISO] Limite de instrucciones alcanzado (" << MAX_INSTR
             << ")." << endl;
        break;
      }

      Instruccion *instr = crearInstruccion(linea);
      if (instr)
        instrucciones[numInstr++] = instr;
    }

    cout << "Cargado: " << archivo << " | Instrucciones: " << numInstr << endl;
    return true;
  }

  // Ciclo Fetch - Decode - Execute
  void ejecutar() {
    if (numInstr == 0) {
      cerr << "[ERROR] No hay instrucciones cargadas." << endl;
      return;
    }

    cpu.activo = true;
    cpu.ICR = 0;

    cout << "\n--- INICIO DE EJECUCION ---" << endl;

    while (cpu.activo && cpu.ICR < numInstr) {
      int pc = cpu.ICR;
      Instruccion *instr = instrucciones[pc];

      // FETCH: mostrar instruccion en curso
      cout << "  [" << pc << "] ";
      instr->imprimir();
      cout << endl;

      // DECODE + EXECUTE (polimorfismo)
      bool continuar = instr->ejecutar(cpu);
      if (!continuar)
        break;

      // Avanzar ICR si la instruccion no lo modifico
      if (cpu.ICR == pc)
        cpu.ICR++;
    }

    cout << "--- FIN DE EJECUCION ---" << endl;
  }

  // Ejecuta y guarda resultado en archivo .out.
  // Redirige cout al archivo .out durante ejecucion y reporte.
  // El prompt de PAUSE sigue apareciendo en la terminal (cerr).
  void ejecutarConSalida(const char *archivo) {
    string nombreOut = generarNombreOut(string(archivo));
    ofstream outFile(nombreOut.c_str());
    if (!outFile.is_open()) {
      cerr << "[ERROR] No se pudo crear: " << nombreOut << endl;
      return;
    }

    // Redirigir cout -> archivo .out
    streambuf *bufOriginal = cout.rdbuf(outFile.rdbuf());

    ejecutar();
    mostrarEstadoCPU();
    mostrarMemoria();

    // Restaurar cout -> consola
    cout.rdbuf(bufOriginal);

    cout << "Resultado guardado en: " << nombreOut << endl;
  }

  // Presentación de resultados
  void mostrarEstadoCPU() const {
    cout << "\n[Registros CPU]" << endl;
    cout << "  ACC = " << cpu.ACC << endl;
    cout << "  ICR = " << cpu.ICR << endl;
    cout << "  MAR = " << cpu.MAR << endl;
    cout << "  MDR = " << cpu.MDR << endl;
    cout << "  UC   = " << (cpu.activo ? "ACTIVA" : "DETENIDA") << endl;
  }

  void mostrarMemoria() const {
    cout << "\n[Memoria Principal - posiciones modificadas]" << endl;
    bool alguna = false;
    for (int i = 0; i < TAM_MEMORIA; i++) {
      if (memoria[i] != 0) {
        cout << "  D" << i << " = " << memoria[i] << endl;
        alguna = true;
      }
    }
    if (!alguna)
      cout << "  (vacia)" << endl;
  }
};

// Programa principal
int main(int argc, char *argv[]) {
  Simulador sim;

  if (argc >= 2) {
    // Ejecutar el archivo indicado como argumento
    if (sim.cargar(argv[1]))
      sim.ejecutarConSalida(argv[1]);
  } else {
    // Ejecutar los tres programas de ejemplo
    const char *progs[] = {"p1.txt", "p2.txt", "p3.txt"};
    const int n = 3;

    for (int i = 0; i < n; i++) {
      cout << "\n======================================" << endl;
      cout << "  PROGRAMA: " << progs[i] << endl;
      cout << "======================================" << endl;
      if (sim.cargar(progs[i]))
        sim.ejecutarConSalida(progs[i]);
    }
  }

  return 0;
}
