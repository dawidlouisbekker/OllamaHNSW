Available for windows Only currently!

For it to work download Ollama: https://ollama.com/download/OllamaSetup.exe
After download run this in your command prompt: ollama pull nomic-embed-text
then you can start the solution in visual studio.

You can use other models to do the embeddings and storage.
Just ensure the size is inside the "size.bin" file in the model's directory. 
Ensure the model is pulled and running before you run the api.CreateEmbdeddings() , I think thats the code it's commented out, to add a file to the database. Also change the file directories.

The database files are in the debug folder. the rest are just tests. you actually only need Source.cpp and hnswlist.h

IF YOU HAVE ANY ISSUES PLEASE MENTION IT
