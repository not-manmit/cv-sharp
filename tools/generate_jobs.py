#!/usr/bin/env python3
"""Generate a synthetic jobs.json dataset for the Job Ranking Engine (V1).

v2: fixes the "repetitive results" bug —
  * compositional descriptions (opening x responsibility x tech x culture)
    assembled per job, so postings are content-unique, not template clones
  * multiple title variants per archetype + seniority labels
  * randomised skill subsets (3-5 core + optional general skill)
  * hard uniqueness guarantee on (title, description)

Deterministic (seeded) so the dataset is reproducible for the report.

Usage:  python tools/generate_jobs.py [--count 2000] [--out backend/data/jobs.json]
"""
import argparse
import json
import random
import os

ARCHETYPES = [
    {
        "key": "backend",
        "titles": ["Backend Software Engineer", "Backend Engineer",
                   "Server-Side Developer", "API Engineer"],
        "skills": ["C++", "Python", "REST APIs", "PostgreSQL", "Docker",
                   "Redis", "gRPC", "Kafka"],
        "opens": [
            "You will own the design and delivery of core backend services.",
            "Join our platform group to build the services behind our products.",
            "We need a backend specialist to evolve our server-side stack.",
        ],
        "resps": [
            "Ship reliable APIs, model data carefully, and keep latency low under load.",
            "Collaborate with product teams to turn requirements into robust services.",
            "Design for scale, review designs with peers, and own services in production.",
        ],
        "techs": [
            "The stack centres on C++ and Python services backed by PostgreSQL.",
            "Day-to-day work spans REST and gRPC services, caching with Redis, and event streaming with Kafka.",
            "Services run in Docker containers with Postgres for storage and Redis for hot data.",
        ],
    },
    {
        "key": "frontend",
        "titles": ["Frontend Developer", "Front-End Engineer",
                   "UI Engineer", "Web Developer"],
        "skills": ["JavaScript", "React", "TypeScript", "CSS", "HTML",
                   "Webpack", "Accessibility", "Redux"],
        "opens": [
            "You will craft the interfaces our users touch every day.",
            "Join the web team to build fast, accessible product experiences.",
            "We are looking for a frontend engineer to own key user journeys.",
        ],
        "resps": [
            "Translate designs into polished, responsive pages and iterate on UX details.",
            "Build reusable component libraries and keep bundle sizes in check.",
            "Pair with designers to ship accessible flows across breakpoints.",
        ],
        "techs": [
            "The front end is React and TypeScript with a design system in CSS.",
            "You will work in React with Redux state, component libraries, and modern build tooling.",
            "Applications are single-page React apps styled with modern CSS and tested in real browsers.",
        ],
    },
    {
        "key": "datasci",
        "titles": ["Data Scientist", "Data Scientist - Analytics",
                   "Insights Scientist", "Quantitative Analyst"],
        "skills": ["Python", "pandas", "Machine Learning", "SQL", "Statistics",
                   "A/B Testing", "Tableau", "Experimentation"],
        "opens": [
            "Turn our data into decisions as a core member of the analytics team.",
            "We need a data scientist to answer product questions with evidence.",
            "Join the insights group and shape what we build next with analysis.",
        ],
        "resps": [
            "Design experiments, measure impact, and present findings to leadership.",
            "Build predictive models and monitor them after they reach production.",
            "Clean messy datasets, explore patterns, and quantify uncertainty honestly.",
        ],
        "techs": [
            "Analysis happens in Python and pandas with data warehouse queries in SQL.",
            "You will use statistical testing, experimentation platforms, and dashboards to communicate results.",
            "Modeling work uses scikit-learn style workflows over SQL-extracted datasets.",
        ],
    },
    {
        "key": "ml",
        "titles": ["Machine Learning Engineer", "ML Engineer",
                   "Deep Learning Engineer", "AI Engineer"],
        "skills": ["Python", "TensorFlow", "PyTorch", "Deep Learning", "NLP",
                   "MLOps", "Transformers", "Computer Vision",
                   "AI", "Generative AI", "Prompt Engineering", "LangChain"],
        "opens": [
            "Build and ship machine learning models that power product features.",
            "Join the ML platform team to take models from notebook to production.",
            "We need an AI engineer for our language and vision roadmap.",
            "Join the AI group and turn artificial intelligence research into shipped product features.",
        ],
        "resps": [
            "Train models, run structured experiments, and improve offline metrics.",
            "Build training and inference pipelines that are reproducible and observable.",
            "Partner with engineers to embed models into latency-sensitive services.",
            "Prototype GenAI capabilities quickly, then harden LLM behaviour with evals and guardrails.",
        ],
        "techs": [
            "Modeling uses PyTorch and TensorFlow with experiment tracking and GPU clusters.",
            "Work covers transformers for NLP tasks and convolutional networks for vision tasks.",
            "Pipelines are Python-based with containerised training jobs and versioned datasets.",
            "Recent projects use large language models with GenAI features, RAG pipelines, prompt engineering and AI agents.",
        ],
    },
    {
        "key": "devops",
        "titles": ["DevOps Engineer", "Infrastructure Engineer",
                   "Platform Engineer", "Cloud Engineer"],
        "skills": ["AWS", "Docker", "Kubernetes", "Terraform", "Linux",
                   "CI/CD", "Monitoring", "Ansible"],
        "opens": [
            "Keep our platform fast, safe, and boring to operate.",
            "Join the infrastructure team and automate everything toil-related.",
            "We need a DevOps engineer to harden and scale our cloud estate.",
        ],
        "resps": [
            "Build self-service pipelines and cut deploy times without cutting corners.",
            "Own clusters, on-call runbooks, and the observability stack.",
            "Codify infrastructure, review changes, and improve reliability metrics.",
        ],
        "techs": [
            "Infrastructure is defined in Terraform on AWS and orchestrated with Kubernetes.",
            "Services run as Docker containers with Prometheus and Grafana for observability.",
            "CI/CD pipelines build, test, and deploy to Linux fleets managed as code.",
        ],
    },
    {
        "key": "mobile",
        "titles": ["Mobile Developer", "iOS Engineer", "Android Engineer",
                   "Mobile App Developer"],
        "skills": ["Swift", "Kotlin", "Android", "iOS", "Flutter",
                   "SwiftUI", "Jetpack Compose", "Mobile UX"],
        "opens": [
            "Build the mobile experience millions of users open every day.",
            "Join the mobile team and ship features across iOS and Android.",
            "We need an engineer to own our cross-platform mobile client.",
        ],
        "resps": [
            "Ship polished screens, smooth animations, and rock-solid offline behaviour.",
            "Instrument crashes and performance, then fix what the data shows.",
            "Work with designers to turn prototypes into store-ready releases.",
        ],
        "techs": [
            "Native clients are written in Swift and Kotlin with shared business logic in Flutter.",
            "iOS work uses SwiftUI while Android uses Jetpack Compose.",
            "Releases flow through fastlane-style pipelines to both app stores.",
        ],
    },
    {
        "key": "qa",
        "titles": ["QA Automation Engineer", "Software Engineer in Test",
                   "Test Automation Engineer", "Quality Engineer"],
        "skills": ["Selenium", "Java", "Test Automation", "CI/CD", "JUnit",
                   "Playwright", "API Testing", "Cypress"],
        "opens": [
            "Guard quality with automation and make regressions expensive to ship.",
            "Join the quality team and build the safety net for our releases.",
            "We need an engineer to expand test coverage across the stack.",
        ],
        "resps": [
            "Author fast, flake-free suites and triage failures before users see them.",
            "Move manual checks into pipelines and track coverage trends.",
            "Test APIs, web flows, and mobile journeys end to end.",
        ],
        "techs": [
            "Automation is Java with JUnit driving browsers through Selenium.",
            "Modern suites use Playwright and Cypress alongside REST API contract tests.",
            "Every commit runs the full suite in CI with clear, actionable reports.",
        ],
    },
    {
        "key": "cloud",
        "titles": ["Cloud Architect", "Solutions Architect",
                   "Enterprise Architect", "Cloud Solutions Architect"],
        "skills": ["AWS", "Azure", "GCP", "Microservices", "Networking",
                   "Security", "Cost Optimisation", "Serverless"],
        "opens": [
            "Define how we build and run systems across multiple clouds.",
            "Join as a cloud architect and set standards teams actually adopt.",
            "We need an architect to balance speed, cost, and resilience.",
        ],
        "resps": [
            "Draft reference architectures, review designs, and evolve standards.",
            "Model costs, reduce waste, and plan migrations with engineering teams.",
            "Design for failure: multi-region, multi-AZ, and secure by default.",
        ],
        "techs": [
            "Estate spans AWS and Azure with infrastructure as code and private networking.",
            "Patterns include microservices, serverless functions, and event-driven integration.",
            "Landing zones enforce identity, logging, and network segmentation policies.",
        ],
    },
    {
        "key": "dataeng",
        "titles": ["Data Engineer", "Analytics Engineer",
                   "Pipeline Engineer", "ETL Engineer"],
        "skills": ["SQL", "Spark", "ETL", "Airflow", "Hadoop",
                   "dbt", "Kafka", "Data Modelling"],
        "opens": [
            "Build the pipelines that make analytics trustworthy.",
            "Join the data platform team and own data from ingest to warehouse.",
            "We need an engineer to scale our batch and streaming pipelines.",
        ],
        "resps": [
            "Design reliable ETL jobs, monitor freshness, and fix data quality issues.",
            "Model warehouse tables for analysts and keep transformations tested.",
            "Stream events with Kafka and backfill history without breaking downstreams.",
        ],
        "techs": [
            "Pipelines are orchestrated in Airflow with transformations written in SQL and Spark.",
            "Batch jobs run on Spark while streaming uses Kafka consumers.",
            "Warehouse modelling follows dbt-style layered transformations with tests.",
        ],
    },
    {
        "key": "pm",
        "titles": ["Product Manager", "Technical Product Manager",
                   "Product Owner", "Group Product Manager"],
        "skills": ["Roadmaps", "Agile", "Stakeholder Management", "Analytics",
                   "A/B Testing", "Prioritisation", "User Research", "GTM"],
        "opens": [
            "Own the product roadmap and the outcomes behind it.",
            "Join the product team and lead discovery for a core surface.",
            "We need a product manager who can bridge customers and engineering.",
        ],
        "resps": [
            "Frame problems, prioritise ruthlessly, and say no with data.",
            "Run discovery, write crisp specs, and align stakeholders weekly.",
            "Define success metrics and iterate with experiments after launch.",
        ],
        "techs": [
            "Backlog runs in Agile two-week sprints with engineering embedded in planning.",
            "Decisions lean on funnel analytics and A/B test readouts.",
            "You will partner daily with design, engineering, and go-to-market teams.",
        ],
    },
    {
        "key": "security",
        "titles": ["Cybersecurity Analyst", "Security Engineer",
                   "SOC Analyst", "Application Security Engineer"],
        "skills": ["SIEM", "Penetration Testing", "Firewalls", "Incident Response",
                   "Vulnerability Management", "Threat Modeling", "IAM", "Reverse Engineering"],
        "opens": [
            "Defend our systems and make attacks boring for adversaries.",
            "Join the security team and cover detection through response.",
            "We need an analyst to harden applications and hunt threats.",
        ],
        "resps": [
            "Triage SIEM alerts, run playbooks, and write post-incident reviews.",
            "Run penetration tests and threat models against new features.",
            "Patch exposures, tune detections, and automate repetitive response steps.",
        ],
        "techs": [
            "Detection runs on SIEM tooling with endpoint telemetry and firewall logs.",
            "AppSec work covers code review, dependency scanning, and secure design reviews.",
            "Response uses documented runbooks with evidence preserved for audits.",
        ],
    },
    {
        "key": "dba",
        "titles": ["Database Administrator", "Database Engineer",
                   "Senior DBA", "Data Platform DBA"],
        "skills": ["SQL", "Oracle", "PostgreSQL", "Performance Tuning", "Backup",
                   "Replication", "Indexing", "Query Optimisation"],
        "opens": [
            "Own the databases the business cannot run without.",
            "Join the data services team and keep queries fast at any hour.",
            "We need a DBA to scale storage, replication, and recovery.",
        ],
        "resps": [
            "Tune slow queries, right-size indexes, and plan capacity ahead of demand.",
            "Automate backups, test restores, and guard uptime through failovers.",
            "Review schemas with application teams before they reach production.",
        ],
        "techs": [
            "Fleet spans PostgreSQL and Oracle with streaming replication and point-in-time recovery.",
            "Performance work uses query plans, index strategies, and workload metrics.",
            "Change management requires rehearsed rollbacks for every risky migration.",
        ],
    },
    {
        "key": "fullstack",
        "titles": ["Full Stack Developer", "Full Stack Engineer",
                   "Product Engineer", "Software Engineer - Web"],
        "skills": ["JavaScript", "Node.js", "React", "MongoDB", "Express",
                   "TypeScript", "GraphQL", "REST APIs"],
        "opens": [
            "Ship features end to end, from database row to rendered pixel.",
            "Join a product squad and own slices across the whole stack.",
            "We need an engineer comfortable on both sides of the API boundary.",
        ],
        "resps": [
            "Build UI, design endpoints, and deploy what you create.",
            "Balance delivery speed with tests, refactors, and documentation.",
            "Work with product to slice projects into small, shippable increments.",
        ],
        "techs": [
            "Front end is React while the API layer is Node.js with Express and MongoDB.",
            "TypeScript runs everywhere with GraphQL stitching services together.",
            "Features ship behind flags with REST endpoints and Mongo-backed persistence.",
        ],
    },
    {
        "key": "systems",
        "titles": ["Systems Programmer", "Systems Engineer",
                   "Low-Level Software Engineer", "Kernel Engineer"],
        "skills": ["C", "C++", "Linux", "Memory Management", "Multithreading",
                   "Performance Optimisation", "Socket Programming", "eBPF"],
        "opens": [
            "Work close to the metal where microseconds matter.",
            "Join the core systems team and build the layer others depend on.",
            "We need a systems engineer for performance-critical components.",
        ],
        "resps": [
            "Profile hot paths, remove locks, and prove wins with benchmarks.",
            "Write memory-safe C++ with clear ownership and rigorous reviews.",
            "Debug production issues across kernels, drivers, and runtimes.",
        ],
        "techs": [
            "Code is portable C and C++ targeting Linux on x86 and ARM.",
            "Concurrency work spans threads, lock-free structures, and io_uring.",
            "Tooling includes perf, sanitizers, and custom tracers for latency work.",
        ],
    },
    {
        "key": "vision",
        "titles": ["Computer Vision Engineer", "Vision ML Engineer",
                   "Image Processing Engineer", "Perception Engineer"],
        "skills": ["OpenCV", "Python", "PyTorch", "Image Processing", "TensorFlow",
                   "Object Detection", "Sensor Fusion", "ONNX"],
        "opens": [
            "Teach machines to see and turn pixels into decisions.",
            "Join the perception team and improve detection under real-world noise.",
            "We need an engineer for our imaging and sensing roadmap.",
        ],
        "resps": [
            "Label wisely, train detectors, and measure error cases honestly.",
            "Optimise models for edge hardware without losing accuracy.",
            "Build data flywheels that collect, version, and audit training images.",
        ],
        "techs": [
            "Pipelines combine OpenCV preprocessing with PyTorch training and ONNX export.",
            "Work covers detection, segmentation, and tracking across camera feeds.",
            "Evaluation uses held-out scenes plus synthetic augmentations for edge cases.",
        ],
    },
    {
        "key": "sre",
        "titles": ["Site Reliability Engineer", "Reliability Engineer",
                   "Production Engineer", "SRE - Tooling"],
        "skills": ["Linux", "Prometheus", "Kubernetes", "Python", "Incident Response",
                   "SLOs", "Terraform", "On-Call"],
        "opens": [
            "Make reliability an engineering outcome, not an accident.",
            "Join the SRE team and own the error budget with product teams.",
            "We need an engineer to automate operations and improve SLOs.",
        ],
        "resps": [
            "Define SLIs and SLOs, then drive work to protect the error budget.",
            "Automate toil away with Python and improve on-call experiences.",
            "Lead incident response, write blameless postmortems, and track action items.",
        ],
        "techs": [
            "Monitoring is Prometheus with Kubernetes underpinning deployments.",
            "Runbooks are executable: scripts first, dashboards second, tickets last.",
            "Capacity planning uses load tests and traffic models updated each quarter.",
        ],
    },
]

GEN_SENTENCES = [
    "You will work in a collaborative team that values clear written communication.",
    "The role offers flexible hours and a budget for conferences and training.",
    "We practice blameless culture and treat incidents as learning opportunities.",
    "Compensation includes equity, health cover, and a home-office allowance.",
    "Hybrid schedule with two days in the office and three remote.",
    "We ship weekly, review carefully, and keep the backlog honest.",
    "The team runs demos every sprint and celebrates finished work.",
    "English is the working language; colleagues span several time zones.",
    "You will mentor juniors and learn from senior engineers in return.",
    "Our product serves millions of users, so quality and observability matter.",
    "Interview process is practical: a coding session and a system discussion.",
    "We keep documentation close to code and update it in the same pull request.",
]

GENERAL_SKILLS = ["Git", "Agile", "Code Review", "Testing", "Communication",
                  "Problem Solving", "Mentoring", "Documentation", "CI/CD",
                  "Debugging"]

COMPANIES = [
    "TechNova", "BrightLabs", "CloudPeak", "DataForge", "NimbusSoft", "CoreStack",
    "PixelDynamics", "QuantumWorks", "BlueOrbit", "Silverline", "Ironclad Systems",
    "Vantage AI", "HexaCore", "WindRiver Tech", "Redwood Digital", "Zenith Labs",
    "Cobalt Infotech", "FusionWare", "Skyline Networks", "Evergreen IT",
    "Apex Circuits", "Lumen Logic", "Vertex Solutions", "NovaBridge", "CipherTech",
]

LOCATIONS = [
    "San Francisco, CA", "New York, NY", "Austin, TX", "Seattle, WA", "Boston, MA",
    "Chicago, IL", "Denver, CO", "Remote (US)", "Atlanta, GA", "Los Angeles, CA",
    "Raleigh, NC", "Portland, OR", "Miami, FL", "Phoenix, AZ", "Columbus, OH",
]

SENIORITY = [
    ("Junior", 0, 2),
    ("", 1, 4),
    ("Senior", 3, 7),
    ("Lead", 5, 10),
    ("Staff", 7, 12),
]


def build_job(rng, job_index, seen):
    """Compose one content-unique job. Retries on (title, description) collision."""
    for _ in range(50):
        arch = ARCHETYPES[job_index % len(ARCHETYPES)]
        # seniority spread deterministically by index, jittered per attempt
        label, lo, hi = SENIORITY[(job_index // len(ARCHETYPES) + rng.randrange(len(SENIORITY))) % len(SENIORITY)]
        years = rng.randint(lo, hi)
        title = (label + " " + rng.choice(arch["titles"])).strip()

        # randomised skill subset: 3-5 core + 0-2 general
        core_n = rng.randint(3, min(5, len(arch["skills"])))
        skills = rng.sample(arch["skills"], core_n)
        if rng.random() < 0.6:
            skills += rng.sample(GENERAL_SKILLS, rng.randint(1, 2))

        # compositional description
        parts = [rng.choice(arch["opens"]), rng.choice(arch["resps"]),
                 rng.choice(arch["techs"])]
        if rng.random() < 0.5:
            parts.append(rng.choice(GEN_SENTENCES))
        description = " ".join(parts)
        if years > 0:
            description += f" Seeking a candidate with {years}+ years of relevant experience."
        else:
            description += " Entry level role; great for recent graduates."
        description += " Core skills: " + ", ".join(skills) + "."

        key = (title, description)
        if key in seen:
            continue  # re-roll sentence combination
        seen.add(key)

        return {
            "id": f"job_{job_index:05d}",
            "title": title,
            "company": rng.choice(COMPANIES),
            "location": rng.choice(LOCATIONS),
            "description": description,
            "skills": skills,
            "experience_years": years,
            "source": "synthetic_v1",
        }
    raise RuntimeError("could not generate a unique job after 50 attempts")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--count", type=int, default=2000)
    parser.add_argument("--out", default=os.path.join(os.path.dirname(__file__), "..", "backend", "data", "jobs.json"))
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    rng = random.Random(args.seed)
    seen = set()
    jobs = [build_job(rng, i, seen) for i in range(args.count)]

    out_path = os.path.abspath(args.out)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(jobs, f, ensure_ascii=False, indent=1)

    titles = {j["title"] for j in jobs}
    print(f"Wrote {len(jobs)} jobs -> {out_path}")
    print(f"  unique titles       : {len(titles)}")
    print(f"  unique descriptions : {len({j['description'] for j in jobs})}")
    print(f"  unique companies    : {len({j['company'] for j in jobs})}")


if __name__ == "__main__":
    main()
