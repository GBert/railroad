Funktions Mapping
=================

Generiert das Funktionsmapping aus Ralf Mendes Git
```
sudo apt install xml2
wget https://raw.githubusercontent.com/RalfMende/LocDbConverterConsole/main/LocDbConverterConsole/Mapping.xml
xml2 < Mapping.xml  | 2csv mappings/mapping @Id @Duration @FunctionTypeCS2 @FunctionTypeZ21
```
